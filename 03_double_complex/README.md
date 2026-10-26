# Double-Precision Complex Solve

## Purpose

Solve a complex system with direct LU factorization, L1 reordering, and a GPU-
resident CSC matrix in double precision. Complex elements have interleaved real and
imaginary components.

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
- [`matrix_complex.mtx`](../matrices/matrix_complex.mtx) — Matrix Market matrix.
- [`rhs_complex_1.vec`](../matrices/rhs_complex_1.vec) — RHS values without a header.

Shared helpers are in [`common/sample_common.h`](../common/sample_common.h).
No `config.json` is needed; configuration uses C API calls in `sample.c`.
The RHS contains `n` real/imaginary pairs for matrix order `n`. Input files store
neither a random seed nor a reference solution. See the
[shared input format](../matrices/PATTERN_FORMAT.md) for index/value types.

The argument order is:

```text
../matrices/matrix_complex.mtx ../matrices/rhs_complex_1.vec
```

Replace inputs from this directory with:

```bash
make run RUN_ARGS='/absolute/path/matrix_complex.mtx /absolute/path/rhs_complex_1.vec'
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
