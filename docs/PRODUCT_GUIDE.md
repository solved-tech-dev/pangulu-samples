# PanguLU C API Guide

## Scope and Package Compatibility

PanguLU solves a sparse square system `A*x = b` using direct LU factorization.
This guide describes the common C object interface used by the samples.
The selected package's `include/pangulu.h` is the authority for declarations
and supported features. Follow that package's README for the GPU and the
toolkit/runtime required by the selected package.

The samples require the PanguLU 26.43 interface with a five-argument
`pangulu_vector_create` and a separate seven-argument
`pangulu_multiple_vector_create`. Check these declarations when choosing a
package; a package with a different vector-constructor signature is incompatible
with these sources. No product implementation or package is included here.

## Objects and Data Types

| Type | Role |
| --- | --- |
| `pangulu_handle_t` | Solver context, licensed-device selection, configuration, and retained factor state |
| `pangulu_matrix_t` | Descriptor for caller-owned sparse matrix arrays |
| `pangulu_vector_t` | Descriptor for caller-owned device RHS or solution storage |

The matrix and vectors must agree on dimensions, value type, and device.
Matrices are square, zero-based CSC or CSR. Samples use scalar block size `1`
and operation `PANGULU_LU`. Compressed pointers use `PANGULU_R_64I` (`int64_t`)
and indices use `PANGULU_R_32I` (`int32_t`). The sample reader limits matrix
order to `INT32_MAX`.

| Value type | Storage per element | Used by |
| --- | --- | --- |
| `PANGULU_R_32F` | One `float` | Real single-precision sample |
| `PANGULU_R_64F` | One `double` | Real double-precision samples |
| `PANGULU_C_32F` | Interleaved real/imaginary `float` components | Complex single-precision sample |
| `PANGULU_C_64F` | Interleaved real/imaginary `double` components | Complex double-precision sample |

Matrices may use `PANGULU_MEMORY_HOST` or `PANGULU_MEMORY_DEVICE` as demonstrated
by the samples. RHS and solution buffers are device-resident. Host matrix input
does not turn factorization or solve into CPU computation.

## Lifecycle and Configuration

Create a handle before allocating storage on its selected device:

```c
pangulu_handle_t h = NULL;
pangulu_status_t status = pangulu_create(gpu_index, &h);
```

`gpu_index` is a zero-based index into the ordered licensed-GPU list, not a
physical device ordinal. `make run` supplies the package-local license through
`PANGULU_LICENSE_PATHS` by default. See
[package and license setup](../libraries/README.md).

Check each API return status before proceeding. Set configuration before the
first execution phase that uses it:

```c
pangulu_config_set(h, PANGULU_REORDERING_METHOD, L1);
```

The samples configure these options:

| Option | Value | Meaning in these samples |
| --- | --- | --- |
| `PANGULU_REORDERING_METHOD` | `L0` | Reordering without MC64 matching |
| `PANGULU_REORDERING_METHOD` | `L1` | Reordering with MC64 matching/scaling |
| `PANGULU_REUSE_LEVEL` | `L1` | Retain factors for repeated RHS solves |
| `PANGULU_REUSE_LEVEL` | `L2` | Retain analysis for matrices with the same sparse pattern |

Reordering and reuse are independent options even when they use the same value
names. These are direct solves, without an iterative method or preconditioner.
No `config.json` is read by these samples; edit the explicit C API configuration
in `sample.c` and clean/rebuild when changing it.

## Matrix and Vector Descriptors

The matrix constructor used here is:

```c
pangulu_matrix_create(&A, layout, n, n, nnz, 1,
                     ptr, idx, values, PANGULU_R_64I, PANGULU_R_32I,
                     value_type, PANGULU_LU, memory_space);
```

For CSC, `ptr` contains column offsets and `idx` contains row indices; for CSR,
these roles become row offsets and column indices. The reader expands Matrix
Market symmetry before descriptor creation. See
[the input format](../matrices/PATTERN_FORMAT.md).

For one contiguous RHS or solution column:

```c
pangulu_vector_create(&B, n, device_b, value_type, PANGULU_MEMORY_DEVICE);
pangulu_vector_create(&X, n, device_x, value_type, PANGULU_MEMORY_DEVICE);
```

For multiple columns, use a separate descriptor for RHS and solution:

```c
pangulu_multiple_vector_create(&B, n, nrhs, ld, device_b,
                              value_type, PANGULU_MEMORY_DEVICE);
pangulu_multiple_vector_create(&X, n, nrhs, ld, device_x,
                              value_type, PANGULU_MEMORY_DEVICE);
```

Dense columns use `data[row + column * ld]`, where `ld >= n` counts value-type
elements, not bytes. Sample 08 uses `nrhs=2` and `ld=n`. Check every constructor's
return status before using its output object.

## Execution Phases

Call `pangulu_execute(handle, phase, A, X, B)` in prerequisite order:

| Phase | Purpose | Vector arguments |
| --- | --- | --- |
| `PANGULU_PHASE_ANALYSIS` | Prepare the sparse structure for factorization | `NULL, NULL` |
| `PANGULU_PHASE_NUMERIC_FACTORISATION` | Compute the first numerical factors | `NULL, NULL` |
| `PANGULU_PHASE_REFACTORISATION` | Recompute factors for changed values with retained analysis | `NULL, NULL` |
| `PANGULU_PHASE_SOLVE` | Solve with existing factors; `X` is the output, `B` the RHS | `X, B` |

Ordinary samples run analysis, numeric factorization, then solve. Sample 08 sets
reuse level `L1`, factorizes once, and solves all RHS columns together. Sample 09
sets reuse level `L2`, analyzes once, factorizes the first matrix, and
refactorizes the second matrix. It solves the RHS vectors separately for each
matrix.

Pattern reuse requires identical compressed pointers and indices, including
ordering; dimensions and nonzero counts alone are insufficient. Sample 09 checks
both arrays before creating solver objects. Do not change the pattern between
analysis and refactorization.

The samples time each library phase in milliseconds. These timings exclude
file input, application data transfers, and independent residual checks.

## Results and Resource Lifetime

`PANGULU_STATUS_SUCCESS` means the API call completed. Other statuses include
not initialized, allocation failure, execution failure, license error, and
unsupported operation; use the selected header for their exact enum names and
available values. Check API status separately from numerical accuracy.

After copying `X` to host memory, the shared helper independently computes
`r = b - A*x` for the original system. It reports the RHS infinity norm and
`||r||_inf / ||b||_inf`, using complex magnitudes for complex vectors. For zero
RHS it checks `||r||_inf` instead. Residual accumulation uses `long double`.
Single-precision samples retain the original double-precision input for checking.

`PASS` requires a finite residual at most `1e-10` for double precision or `1e-5`
for single precision; the absolute residual uses the same limit for zero RHS.
Invalid/non-finite data, a residual allocation failure, or an excessive residual
returns failure. Exit code `0` is reached only after execution and all required
checks succeed. These example tolerances do not replace your application's
accuracy requirements. The single-precision limit is selected in those samples
with `SAMPLE_RESIDUAL_TOLERANCE`; the shared default is the double-precision
limit.

Arrays passed to descriptors remain caller-owned. Keep them valid while the
objects reference them. After execution and verification:

1. Destroy vector descriptors with `pangulu_vector_destroy`.
2. Destroy matrix descriptors with `pangulu_matrix_destroy`.
3. Destroy the handle with `pangulu_destroy`.
4. Release application-owned device and host buffers.

The complete error and cleanup paths are shown in each `sample.c`. For user
setup and run commands, return to the [repository README](../README.md).
