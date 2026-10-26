/* PanguLU sample 08: double precision, CSC, GPU matrix, L1 factorization
 * (reuse level L1) with two RHS columns in one SOLVE call. */
#include "sample_common.h"

int main(int argc, char **argv)
{
    /* Select an index in the ordered licensed-GPU list, not a CUDA ordinal. */
    int gpu_index = 0, arg = 1;
    if (argc >= 3 && strcmp(argv[1], "-g") == 0)
    {
        gpu_index = atoi(argv[2]);
        arg = 3;
    }
    if (argc - arg != 3)
    {
        fprintf(stderr, "usage: %s [-g gpu] matrix.mtx rhs1.vec rhs2.vec\n",
                argv[0]);
        return 2;
    }
    const char *matrix_path = argv[arg];
    const char *rhs1_path = argv[arg + 1];
    const char *rhs2_path = argv[arg + 2];

    /* Read the matrix into host arrays; convert file indices to zero-based form. */
    spmat_t a;
    if (read_mtx_real(matrix_path, 0, &a) != 0)
    {
        fprintf(stderr, "cannot read %s\n", matrix_path);
        return 2;
    }
    const int64_t n = a.n;
    double *b = (double *)calloc((size_t)n * 2u, sizeof(double));
    double *x = (double *)calloc((size_t)n * 2u, sizeof(double));
    if (b == NULL || x == NULL)
    {
        return 2;
    }

    printf("PanguLU sample 08: double, CSC, GPU matrix, L1 factor + 2 RHS\n");
    printf("matrix=%s (order %lld, nnz %lld)\n", matrix_path, (long long)n,
           (long long)a.nnz);
    printf("rhs1=%s\nrhs2=%s\n", rhs1_path, rhs2_path);

    /* Keep API objects and application-owned buffers separate for cleanup. */
    pangulu_handle_t h = NULL;
    pangulu_matrix_t A = NULL;
    pangulu_vector_t X = NULL, B = NULL;
    int64_t *d_ptr = NULL;
    int32_t *d_idx = NULL;
    double *d_values = NULL, *d_b = NULL, *d_x = NULL;
    int rc = 1;

    /* Create the licensed handle before allocating data on its selected GPU. */
    if (pangulu_create(gpu_index, &h) != PANGULU_STATUS_SUCCESS)
        goto done;
    /* Reordering L1 enables matching/scaling; reuse L1 retains the factors.
     * Reordering and reuse are independent configuration options. */
    if (pangulu_config_set(h, PANGULU_REORDERING_METHOD, L1) !=
            PANGULU_STATUS_SUCCESS ||
        pangulu_config_set(h, PANGULU_REUSE_LEVEL, L1) != PANGULU_STATUS_SUCCESS)
        goto done;
    /* Upload compressed pointers, indices, and values to the selected GPU. */
    if (upload_matrix_device(a.ptr, a.idx, a.values, n, a.nnz,
                             sizeof(double) * (size_t)a.nnz, &d_ptr, &d_idx,
                             (void **)&d_values) != 0)
        goto done;
    /* RHS and solution always use separate GPU buffers, even for host matrix input. */
    if (cudaMalloc((void **)&d_b, sizeof(double) * (size_t)n * 2u) != cudaSuccess ||
        cudaMalloc((void **)&d_x, sizeof(double) * (size_t)n * 2u) != cudaSuccess ||
        cudaMemset(d_x, 0, sizeof(double) * (size_t)n * 2u) != cudaSuccess)
        goto done;
    /* Describe existing arrays; the application retains their ownership. */
    if (pangulu_matrix_create(&A, PANGULU_MATRIX_LAYOUT_CSC, n, n, a.nnz, 1,
                              d_ptr, d_idx, d_values, PANGULU_R_64I,
                              PANGULU_R_32I, PANGULU_R_64F, PANGULU_LU,
                              PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS)
        goto done;
    /* Two column-major columns, length=n and ld=n; ld counts elements, not bytes. */
    if (pangulu_multiple_vector_create(&B, n, 2, n, d_b, PANGULU_R_64F,
                              PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS ||
        pangulu_multiple_vector_create(&X, n, 2, n, d_x, PANGULU_R_64F,
                              PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS)
        goto done;

    /* One factorization, then one solve for both RHS columns. */
    {
        const pangulu_phase_t pre[2] = { PANGULU_PHASE_ANALYSIS,
                                         PANGULU_PHASE_NUMERIC_FACTORISATION };
        const char *pre_names[2] = { "Analysis", "Numeric factorisation" };
        for (int i = 0; i < 2; ++i)
        {
            const double t0 = wall_seconds();
            const pangulu_status_t s = pangulu_execute(h, pre[i], A, NULL, NULL);
            const double dt = wall_seconds() - t0;
            if (s == PANGULU_STATUS_SUCCESS)
                printf("%s time: %.3f ms\n", pre_names[i], dt * 1.0e3);
            if (s != PANGULU_STATUS_SUCCESS)
            {
                fprintf(stderr, "phase %s failed status %d\n", pre_names[i],
                        (int)s);
                goto done;
            }
        }
    }

    for (int k = 0; k < 2; ++k)
    {
        const char *rhs_path = k == 0 ? rhs1_path : rhs2_path;
        if (read_vec_real(rhs_path, n, b + (size_t)k * (size_t)n) != 0)
        {
            fprintf(stderr, "cannot read %s\n", rhs_path);
            goto done;
        }
    }
    if (cudaMemcpy(d_b, b, sizeof(double) * (size_t)n * 2u,
                   cudaMemcpyHostToDevice) != cudaSuccess)
        goto done;
    if (run_solve(h, A, X, B) != 0)
        goto done;
    /* Download only for application-side validation against the original system. */
    if (cudaMemcpy(x, d_x, sizeof(double) * (size_t)n * 2u,
                   cudaMemcpyDeviceToHost) != cudaSuccess)
        goto done;
    for (int k = 0; k < 2; ++k)
    {
        printf("rhs %d: ", k + 1);
        /* Report residual quality separately from API completion; retain NaN/Inf. */
        if (report_residual(&a, x + (size_t)k * (size_t)n,
                             b + (size_t)k * (size_t)n) != 0)
            goto done;
    }
    rc = 0;
/* Destroy descriptors and handle before freeing the arrays they reference. */
done:
    if (X)
        pangulu_vector_destroy(X);
    if (B)
        pangulu_vector_destroy(B);
    if (A)
        pangulu_matrix_destroy(A);
    if (h)
        pangulu_destroy(h);
    cudaFree(d_x);
    cudaFree(d_b);
    cudaFree(d_values);
    cudaFree(d_idx);
    cudaFree(d_ptr);
    free(x);
    free(b);
    spmat_free(&a);
    return rc;
}
