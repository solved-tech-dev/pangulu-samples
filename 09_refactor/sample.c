/* PanguLU sample 09: double precision, CSC, GPU matrices.  One analysis,
 * then two matrices with the same sparsity pattern are factorised
 * (numeric + refactorisation, reuse level L2) and each is solved for two
 * different right-hand sides. */
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
    if (argc - arg != 6)
    {
        fprintf(stderr,
                "usage: %s [-g gpu] m1.mtx r1a.vec r1b.vec m2.mtx r2a.vec "
                "r2b.vec\n",
                argv[0]);
        return 2;
    }
    const char *m1_path = argv[arg], *r1a = argv[arg + 1], *r1b = argv[arg + 2];
    const char *m2_path = argv[arg + 3], *r2a = argv[arg + 4],
               *r2b = argv[arg + 5];

    /* Both inputs must have identical compressed pointers and row indices.
     * Equal dimensions and nnz alone do not establish pattern compatibility. */
    spmat_t a1, a2;
    if (read_mtx_real(m1_path, 0, &a1) != 0 || read_mtx_real(m2_path, 0, &a2) != 0)
    {
        fprintf(stderr, "cannot read matrices\n");
        return 2;
    }
    if (a1.n != a2.n || a1.nnz != a2.nnz ||
        memcmp(a1.ptr, a2.ptr, ((size_t)a1.n + 1) * sizeof(*a1.ptr)) != 0 ||
        memcmp(a1.idx, a2.idx, (size_t)a1.nnz * sizeof(*a1.idx)) != 0)
    {
        fprintf(stderr, "the two matrices must share the same sparsity pattern\n");
        spmat_free(&a2);
        spmat_free(&a1);
        return 2;
    }
    const int64_t n = a1.n;
    double *b = (double *)calloc((size_t)n, sizeof(double));
    double *x = (double *)calloc((size_t)n, sizeof(double));
    if (b == NULL || x == NULL)
    {
        return 2;
    }

    printf("PanguLU sample 09: double, CSC, GPU, refactorisation (2 matrices x 2 RHS)\n");
    printf("matrix1=%s matrix2=%s (order %lld, nnz %lld)\n", m1_path, m2_path,
           (long long)n, (long long)a1.nnz);

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
    /* Reordering L1 enables matching/scaling; reuse L2 permits refactorization.
     * Configure both before the first analysis. */
    if (pangulu_config_set(h, PANGULU_REORDERING_METHOD, L1) !=
            PANGULU_STATUS_SUCCESS ||
        pangulu_config_set(h, PANGULU_REUSE_LEVEL, L2) != PANGULU_STATUS_SUCCESS)
        goto done;
    /* RHS and solution always use separate GPU buffers, even for host matrix input. */
    if (cudaMalloc((void **)&d_b, sizeof(double) * (size_t)n) != cudaSuccess ||
        cudaMalloc((void **)&d_x, sizeof(double) * (size_t)n) != cudaSuccess ||
        cudaMemset(d_x, 0, sizeof(double) * (size_t)n) != cudaSuccess)
        goto done;
    /* The five-argument constructor describes a single contiguous GPU vector. */
    if (pangulu_vector_create(&B, n, d_b, PANGULU_R_64F,
                              PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS ||
        pangulu_vector_create(&X, n, d_x, PANGULU_R_64F,
                              PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS)
        goto done;

    for (int mi = 0; mi < 2; ++mi)
    {
        const spmat_t *cur = mi == 0 ? &a1 : &a2;
        /* Upload compressed pointers, indices, and values to the selected GPU. */
        if (upload_matrix_device(cur->ptr, cur->idx, cur->values, n, cur->nnz,
                                 sizeof(double) * (size_t)cur->nnz, &d_ptr,
                                 &d_idx, (void **)&d_values) != 0)
            goto done;
        /* Describe existing arrays; the application retains their ownership. */
        if (pangulu_matrix_create(&A, PANGULU_MATRIX_LAYOUT_CSC, n, n, cur->nnz,
                                  1, d_ptr, d_idx, d_values, PANGULU_R_64I,
                                  PANGULU_R_32I, PANGULU_R_64F, PANGULU_LU,
                                  PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS)
            goto done;

        pangulu_phase_t factor_phase;
        const char *factor_name;
        if (mi == 0)
        {
            /* First matrix: analysis then numeric factorisation. */
            const double t0 = wall_seconds();
            pangulu_status_t s = pangulu_execute(h, PANGULU_PHASE_ANALYSIS, A,
                                                 NULL, NULL);
            const double dt = wall_seconds() - t0;
            if (s == PANGULU_STATUS_SUCCESS)
                printf("Analysis time: %.3f ms\n", dt * 1.0e3);
            if (s != PANGULU_STATUS_SUCCESS)
            {
                fprintf(stderr, "analysis failed status %d\n", (int)s);
                goto done;
            }
            factor_phase = PANGULU_PHASE_NUMERIC_FACTORISATION;
            factor_name = "Numeric factorisation";
        }
        else
        {
            /* Second matrix: same pattern, refactorisation only. */
            factor_phase = PANGULU_PHASE_REFACTORISATION;
            factor_name = "Refactorisation";
        }
        {
            const double t0 = wall_seconds();
            pangulu_status_t s =
                pangulu_execute(h, factor_phase, A, NULL, NULL);
            const double dt = wall_seconds() - t0;
            if (s == PANGULU_STATUS_SUCCESS)
                printf("%s time: %.3f ms\n", factor_name, dt * 1.0e3);
            if (s != PANGULU_STATUS_SUCCESS)
            {
                fprintf(stderr, "%s failed status %d\n", factor_name, (int)s);
                goto done;
            }
        }

        const char *rhs_paths[2] = { mi == 0 ? r1a : r2a, mi == 0 ? r1b : r2b };
        for (int k = 0; k < 2; ++k)
        {
            if (read_vec_real(rhs_paths[k], n, b) != 0)
            {
                fprintf(stderr, "cannot read %s\n", rhs_paths[k]);
                goto done;
            }
            if (cudaMemcpy(d_b, b, sizeof(double) * (size_t)n,
                           cudaMemcpyHostToDevice) != cudaSuccess)
                goto done;
            if (run_solve(h, A, X, B) != 0)
                goto done;
            /* Download only for application-side validation against the original system. */
            if (cudaMemcpy(x, d_x, sizeof(double) * (size_t)n,
                           cudaMemcpyDeviceToHost) != cudaSuccess)
                goto done;
            printf("matrix %d rhs %d: ", mi + 1, k + 1);
            /* Report residual quality separately from API completion; retain NaN/Inf. */
            if (report_residual(cur, x, b) != 0)
                goto done;
        }

        if (A)
        {
            pangulu_matrix_destroy(A);
            A = NULL;
        }
        cudaFree(d_values);
        cudaFree(d_idx);
        cudaFree(d_ptr);
        d_values = NULL;
        d_idx = NULL;
        d_ptr = NULL;
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
    spmat_free(&a2);
    spmat_free(&a1);
    return rc;
}
