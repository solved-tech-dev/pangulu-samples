# Refactorization and Factor Reuse

## Purpose

Analyze a real, double-precision CSC matrix once with L1 reordering and reuse level
L2. Factorize the first matrix, refactorize a second matrix with the same sparse
pattern, and solve two RHS vectors separately for each matrix.

This sample uses a direct solver; no iterative method or preconditioner is
configured.

## Build and Run

Complete the [package and license setup](../README.md#library-package-and-license)
first. Use Linux with the GPU and the toolkit/runtime required by the selected
package. From this directory:

```bash
make
make run
make clean
```

For a package outside the configured default location:

```bash
export PANGULU_LICENSE_PATHS="/path/to/package/license.key"
make SDK_ROOT=/path/to/package
make run SDK_ROOT=/path/to/package
```

`SDK_ROOT` selects the package directory; `PANGULU_SDK` supplies that path through
the environment. `make run` sets package/runtime library search paths.
`GPU_INDEX` defaults to the first entry in the licensed-GPU list, not a physical
device ordinal. Run `make clean` before changing source, package, or build settings.

## Input Files

- [`sample.c`](sample.c) — complete C API workflow and configuration.
- [`sample_common.h`](sample_common.h) — input, transfer, timing, and residual helpers.
- [`matrix_real.mtx`](data/matrix_real.mtx) — Matrix Market coordinate matrix.
- [`rhs_real_1.vec`](data/rhs_real_1.vec) — Plain-text RHS values, without a size header.
- [`rhs_real_2.vec`](data/rhs_real_2.vec) — Plain-text RHS values, without a size header.
- [`matrix_real_b.mtx`](data/matrix_real_b.mtx) — Matrix Market coordinate matrix.
- [`rhs_real_b_1.vec`](data/rhs_real_b_1.vec) — Plain-text RHS values, without a size header.
- [`rhs_real_b_2.vec`](data/rhs_real_b_2.vec) — Plain-text RHS values, without a size header.

No `config.json` is used; configuration is set through C API calls in `sample.c`.
The matrix reader converts one-based file indices to zero-based compressed
storage with signed 64-bit pointers and signed 32-bit indices. RHS input contains
`n` real values, where `n` is the matrix order. Inputs are fixed text files;
no random seed or reference-solution field is stored.

The second matrix must have the same sparse pattern, including compressed
pointers and indices, as the first. Only numerical values may change. Equal
order and entry count alone do not establish compatibility.

The default argument order is:

```text
data/matrix_real.mtx data/rhs_real_1.vec data/rhs_real_2.vec data/matrix_real_b.mtx data/rhs_real_b_1.vec data/rhs_real_b_2.vec
```

To use your own inputs from this directory, keep that order:

```bash
make run RUN_ARGS='/absolute/path/matrix_real.mtx /absolute/path/rhs_real_1.vec /absolute/path/rhs_real_2.vec /absolute/path/matrix_real_b.mtx /absolute/path/rhs_real_b_1.vec /absolute/path/rhs_real_b_2.vec'
```

See the [input format description](../README.md#sample-input) for supported
matrix fields, symmetry handling, layouts, and precision.

## Output

Output includes input paths, matrix order/nonzero count, library phase times
in milliseconds, and an independently computed original-system residual.
Output includes one analysis, initial factorization, refactorization, and
separate solve timings and residuals for each matrix/RHS pair.

The residual is `||b - A*x||_inf / ||b||_inf`, using complex magnitudes for complex
vectors. A zero RHS instead produces the absolute infinity norm with a note.

API success does not establish numerical accuracy. These samples report residuals
without a built-in tolerance or `PASS` label. Accept numerical **PASS** only when
every original-system residual is finite and within your application's tolerance.
Exit code `0` means execution and reporting completed; inspect the residual even
then. A nonzero exit code indicates an input, allocation, or execution failure.
See [Output and Verification](../README.md#output-and-verification) and
[Troubleshooting](../README.md#troubleshooting) for details.
