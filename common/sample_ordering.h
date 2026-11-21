#ifndef SAMPLE_ORDERING_H
#define SAMPLE_ORDERING_H

#include "sample_common.h"

typedef struct
{
    int64_t order;
    int *rows;
    int *columns;
} sample_permutation_t;

static int ordering_status(const char *operation, pangulu_status_t status)
{
    if (status == PANGULU_STATUS_SUCCESS)
        return 0;
    fprintf(stderr, "%s failed (PanguLU status %d)\n", operation, (int)status);
    return -1;
}

/* These small examples use the original real CSR matrix for the L2 residual. */
static int check_ordering_solution(const spmat_t *a, const double *x, const double *b)
{
    long double error = 0, rhs_norm = 0;
    for (int64_t row = 0; row < a->n; ++row)
    {
        long double ax = 0;
        for (int64_t k = a->ptr[row]; k < a->ptr[row + 1]; ++k)
            ax += (long double)a->values[k] * x[a->idx[k]];
        error = hypotl(error, ax - b[row]);
        rhs_norm = hypotl(rhs_norm, b[row]);
    }

    if (!isfinite(rhs_norm) || rhs_norm == 0)
    {
        fprintf(stderr, "relative L2 residual is undefined for this RHS\n");
        return -1;
    }
    const long double residual = error / rhs_norm;
    const int passed = isfinite(residual) && residual <= 1.0e-10L;
    printf("||Ax - b||_2 / ||b||_2 = %.12Le\n", residual);
    printf("verification: %s (tolerance 1e-10)\n", passed ? "PASS" : "FAIL");
    return passed ? 0 : -1;
}

static int run_ordering_sample(int argc, char **argv, pangulu_reordering_algo_t algorithm,
                               const char *name, const sample_permutation_t *permutation)
{
    int gpu_index = 0, arg = 1;
    if (argc >= 3 && strcmp(argv[1], "-g") == 0)
    {
        char *end = NULL;
        const long index = strtol(argv[2], &end, 10);
        if (*argv[2] == '\0' || *end != '\0' || index < 0 || index > INT32_MAX)
        {
            fprintf(stderr, "invalid licensed GPU index: %s\n", argv[2]);
            return 2;
        }
        gpu_index = (int)index;
        arg = 3;
    }
    if (argc - arg != 1)
    {
        fprintf(stderr, "usage: %s [-g gpu] matrix.mtx\n", argv[0]);
        return 2;
    }

    spmat_t a;
    if (read_mtx_real(argv[arg], 1, &a) != 0)
    {
        fprintf(stderr, "cannot read a square real matrix from %s\n", argv[arg]);
        return 2;
    }
    if (permutation && permutation->order != a.n)
    {
        fprintf(stderr, "the supplied row and column maps require order %lld\n",
                (long long)permutation->order);
        spmat_free(&a);
        return 2;
    }

    int result = 2;
    double *b = calloc((size_t)a.n, sizeof(*b));
    double *x = calloc((size_t)a.n, sizeof(*x));
    const size_t vector_bytes = sizeof(*b) * (size_t)a.n;
    pangulu_handle_t handle = NULL;
    pangulu_matrix_t matrix = NULL;
    pangulu_vector_t rhs = NULL, solution = NULL;
    int64_t *device_ptr = NULL;
    int32_t *device_index = NULL;
    void *device_values = NULL, *device_b = NULL, *device_x = NULL;

    if (!b || !x)
    {
        fprintf(stderr, "cannot allocate host vectors\n");
        goto done;
    }
    /* Pick x_ref = all ones, so b is the original row-sum vector. */
    for (int64_t row = 0; row < a.n; ++row)
    {
        long double sum = 0;
        for (int64_t k = a.ptr[row]; k < a.ptr[row + 1]; ++k)
            sum += a.values[k];
        b[row] = (double)sum;
    }
    printf("PanguLU ordering %s, matrix=%s, order=%lld, nnz=%lld\n",
           name, argv[arg], (long long)a.n, (long long)a.nnz);

    result = 1;
    if (ordering_status("create handle", pangulu_create(gpu_index, &handle)) ||
        /* Isolate the selected ordering from L1 matching and scaling. */
        ordering_status("set L0 matching policy",
                        pangulu_config_set(handle, PANGULU_REORDERING_METHOD, L0)) ||
        ordering_status("select ordering", pangulu_reordering_algo_set(handle, algorithm)))
        goto done;

    /* These wrappers perform the matching backend's cudaMalloc/hipMalloc and
     * cudaMemcpy/hipMemcpy operations. Native device allocations are also valid
     * with PANGULU_MEMORY_DEVICE when they belong to the handle's GPU. */
    if (upload_matrix_device(handle, a.ptr, a.idx, a.values, a.n, a.nnz,
                             (size_t)a.nnz * sizeof(*a.values),
                             &device_ptr, &device_index, &device_values))
    {
        fprintf(stderr, "matrix device upload failed\n");
        goto done;
    }
    if (ordering_status("allocate device RHS", pangulu_device_alloc(handle, &device_b, vector_bytes)) ||
        ordering_status("allocate device solution", pangulu_device_alloc(handle, &device_x, vector_bytes)) ||
        ordering_status("upload RHS", pangulu_device_copy(handle, device_b, b, vector_bytes,
                                                           PANGULU_COPY_HOST_TO_DEVICE)) ||
        ordering_status("create CSR descriptor",
                        pangulu_matrix_create_sym(&matrix, PANGULU_MATRIX_LAYOUT_CSR,
                            a.n, a.n, a.nnz, 1, device_ptr, device_index, device_values,
                            PANGULU_R_64I, PANGULU_R_32I, PANGULU_R_64F, PANGULU_MATRIX_GENERAL,
                            PANGULU_MATRIX_VIEW_FULL, PANGULU_MEMORY_DEVICE)) ||
        ordering_status("create RHS descriptor",
                        pangulu_vector_create(&rhs, a.n, device_b, PANGULU_R_64F,
                                              PANGULU_MEMORY_DEVICE)) ||
        ordering_status("create solution descriptor",
                        pangulu_vector_create(&solution, a.n, device_x, PANGULU_R_64F,
                                              PANGULU_MEMORY_DEVICE)))
        goto done;

    if (permutation)
    {
        /* The first analysis supplies the matrix order to the handle. */
        pangulu_status_t status =
            pangulu_execute(handle, PANGULU_PHASE_ANALYSIS, matrix, NULL, NULL);
        if (status != PANGULU_STATUS_REORDERING_PERM_NOT_SET)
        {
            fprintf(stderr, "expected REORDERING_PERM_NOT_SET, got %d\n", (int)status);
            goto done;
        }
        if (ordering_status("set row and column permutations",
                            pangulu_reordering_perm_set(handle, permutation->rows,
                                                        permutation->columns)))
            goto done;
    }

    if (run_analysis_numeric_solve(handle, matrix, solution, rhs) ||
        ordering_status("download solution", pangulu_device_copy(handle, x, device_x, vector_bytes,
                                                                 PANGULU_COPY_DEVICE_TO_HOST)) ||
        check_ordering_solution(&a, x, b))
        goto done;
    result = 0;

done:
    if (solution)
        pangulu_vector_destroy(solution);
    if (rhs)
        pangulu_vector_destroy(rhs);
    if (matrix)
        pangulu_matrix_destroy(matrix);
    if (handle)
    {
        /* The wrapper performs cudaFree/hipFree while the handle is valid. */
        pangulu_device_free(handle, device_x);
        pangulu_device_free(handle, device_b);
        pangulu_device_free(handle, device_values);
        pangulu_device_free(handle, device_index);
        pangulu_device_free(handle, device_ptr);
        pangulu_destroy(handle);
    }
    free(x);
    free(b);
    spmat_free(&a);
    return result;
}

#endif /* SAMPLE_ORDERING_H */
