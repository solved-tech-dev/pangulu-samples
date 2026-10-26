/* PanguLU sample 04: single complex, CSC, GPU matrix, L1 reordering. */
#define SAMPLE_RESIDUAL_TOLERANCE 1.0e-5L
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
    if (argc - arg != 2)
    {
        fprintf(stderr, "usage: %s [-g gpu] matrix.mtx rhs.vec\n", argv[0]);
        return 2;
    }
    const char *matrix_path = argv[arg], *rhs_path = argv[arg + 1];

    /* Read the matrix into host arrays; convert file indices to zero-based form. */
    spmat_t a;
    if (read_mtx_complex(matrix_path, &a) != 0)
    {
        fprintf(stderr, "cannot read %s\n", matrix_path);
        return 2;
    }
    const int64_t n = a.n;
    double *b = (double *)calloc(2u * (size_t)n, sizeof(double));
    double *x = (double *)calloc(2u * (size_t)n, sizeof(double));
    if (b == NULL || x == NULL || read_vec_complex(rhs_path, n, b) != 0)
    {
        fprintf(stderr, "cannot read %s\n", rhs_path);
        return 2;
    }

    /* Cast input for single-precision computation; retain originals for validation. */
    float *fvalues = to_float32(a.values, 2 * a.nnz);
    float *fb = to_float32(b, 2 * n);
    float *fx = (float *)calloc(2u * (size_t)n, sizeof(float));
    if (fvalues == NULL || fb == NULL || fx == NULL)
    {
        fprintf(stderr, "float buffer allocation failed\n");
        return 2;
    }

    printf("PanguLU sample 04: float complex, CSC, GPU matrix, L1 reordering\n");
    printf("matrix=%s (order %lld, nnz %lld)\n", matrix_path, (long long)n,
           (long long)a.nnz);
    printf("rhs=%s\n", rhs_path);

    /* Keep API objects and application-owned buffers separate for cleanup. */
    pangulu_handle_t h = NULL;
    pangulu_matrix_t A = NULL;
    pangulu_vector_t X = NULL, B = NULL;
    int64_t *d_ptr = NULL;
    int32_t *d_idx = NULL;
    float *d_values = NULL, *d_b = NULL, *d_x = NULL;
    int rc = 1;

    /* Create the licensed handle before allocating data on its selected GPU. */
    if (pangulu_create(gpu_index, &h) != PANGULU_STATUS_SUCCESS)
        goto done;
    /* Enable MC64 matching and scaling through the L1 reordering option. */
    if (pangulu_config_set(h, PANGULU_REORDERING_METHOD, L1) !=
        PANGULU_STATUS_SUCCESS)
        goto done;
    /* Upload compressed pointers, indices, and values to the selected GPU. */
    if (upload_matrix_device(a.ptr, a.idx, fvalues, n, a.nnz,
                             2u * sizeof(float) * (size_t)a.nnz, &d_ptr, &d_idx,
                             (void **)&d_values) != 0)
        goto done;
    /* RHS and solution always use separate GPU buffers, even for host matrix input. */
    if (cudaMalloc((void **)&d_b, 2u * sizeof(float) * (size_t)n) !=
            cudaSuccess ||
        cudaMalloc((void **)&d_x, 2u * sizeof(float) * (size_t)n) !=
            cudaSuccess ||
        cudaMemcpy(d_b, fb, 2u * sizeof(float) * (size_t)n,
                   cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemset(d_x, 0, 2u * sizeof(float) * (size_t)n) != cudaSuccess)
        goto done;
    /* Describe existing arrays; the application retains their ownership. */
    if (pangulu_matrix_create(&A, PANGULU_MATRIX_LAYOUT_CSC, n, n, a.nnz, 1,
                              d_ptr, d_idx, d_values, PANGULU_R_64I,
                              PANGULU_R_32I, PANGULU_C_32F, PANGULU_LU,
                              PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS)
        goto done;
    /* The five-argument constructor describes a single contiguous GPU vector. */
    if (pangulu_vector_create(&B, n, d_b, PANGULU_C_32F,
                              PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS ||
        pangulu_vector_create(&X, n, d_x, PANGULU_C_32F,
                              PANGULU_MEMORY_DEVICE) != PANGULU_STATUS_SUCCESS)
        goto done;
    /* Execute and time ANALYSIS -> NUMERIC_FACTORISATION -> SOLVE. */
    if (run_analysis_numeric_solve(h, A, X, B) != 0)
        goto done;
    if (cudaMemcpy(fx, d_x, 2u * sizeof(float) * (size_t)n,
                   cudaMemcpyDeviceToHost) != cudaSuccess)
        goto done;
    for (int64_t i = 0; i < 2 * n; ++i)
        x[i] = (double)fx[i];
    /* Report residual quality separately from API completion; retain NaN/Inf. */
    if (report_residual(&a, x, b) != 0)
        goto done;
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
    free(fx);
    free(fb);
    free(fvalues);
    free(x);
    free(b);
    spmat_free(&a);
    return rc;
}
