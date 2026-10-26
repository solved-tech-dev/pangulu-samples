# Refactorization and Factor Reuse

## Purpose

Analyze a real, double-precision CSC matrix once with L1 reordering and reuse level
L2. Factorize the first matrix, refactorize a second matrix with the same sparse
pattern, and solve two RHS vectors separately for each matrix.

## Build and Run

Complete the [package and license setup](../README.md#library-package-and-license)
first. From this directory:

```bash
make
make run
make clean
```

For a custom package location:

```bash
make LIBRARY_ROOT=/path/to/package
make run LIBRARY_ROOT=/path/to/package
```

Use the GPU and the toolkit/runtime required by the selected package.
`make run` sets the package/runtime search paths and uses that package's
`license.key` by default. `GPU_INDEX` selects an entry in the licensed-GPU list.
Run `make clean` before changing the source, package, or build environment.

## Input Files

- [`sample.c`](sample.c) — complete C API calls and configuration.
- [`matrix_real.mtx`](../matrices/matrix_real.mtx) — Matrix Market matrix.
- [`rhs_real_1.vec`](../matrices/rhs_real_1.vec) — RHS values without a header.
- [`rhs_real_2.vec`](../matrices/rhs_real_2.vec) — RHS values without a header.
- [`matrix_real_b.mtx`](../matrices/matrix_real_b.mtx) — Matrix Market matrix.
- [`rhs_real_b_1.vec`](../matrices/rhs_real_b_1.vec) — RHS values without a header.
- [`rhs_real_b_2.vec`](../matrices/rhs_real_b_2.vec) — RHS values without a header.

Shared helpers are in [`common/sample_common.h`](../common/sample_common.h).
No `config.json` is needed; configuration uses C API calls in `sample.c`.
The RHS contains `n` real values for matrix order `n`. Input files store
neither a random seed nor a reference solution. See the
[shared input format](../matrices/PATTERN_FORMAT.md) for index/value types.

Both matrices must have identical compressed pointers and indices, with only
values changing. Analysis runs once; the second matrix is refactorized, and
each matrix/RHS pair is verified separately.

The argument order is:

```text
../matrices/matrix_real.mtx ../matrices/rhs_real_1.vec ../matrices/rhs_real_2.vec ../matrices/matrix_real_b.mtx ../matrices/rhs_real_b_1.vec ../matrices/rhs_real_b_2.vec
```

Replace inputs from this directory with:

```bash
make run RUN_ARGS='/absolute/path/matrix_real.mtx /absolute/path/rhs_real_1.vec /absolute/path/rhs_real_2.vec /absolute/path/matrix_real_b.mtx /absolute/path/rhs_real_b_1.vec /absolute/path/rhs_real_b_2.vec'
```

## Output

Output includes matrix order/nonzero count and phase times in milliseconds.
The host independently checks `r = b - A*x` using the original input and prints
`||r||_inf / ||b||_inf` and the RHS infinity norm. Complex norms use complex
magnitudes; zero RHS uses the absolute residual with a note.

`PASS` requires a finite original-system residual no greater than `1e-10`
(absolute for zero RHS). API success alone does not meet this requirement.
Exit code `0` means the execution and all required residual checks passed;
nonzero indicates an input, execution, or verification failure. See
[verification details](../README.md#output-and-verification) and
[troubleshooting](../docs/TROUBLESHOOTING.md).
