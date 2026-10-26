# Refactorization and Factor Reuse

Use reuse level L2 to analyze once, factorize the first matrix, and refactorize a second matrix with the same sparse pattern. Solve two RHS vectors separately for each matrix.

## Build and run

Run from this directory on a compatible Linux/CUDA system:

```bash
export PANGULU_LICENSE_PATHS="/absolute/path/to/license.key"
make SDK_ROOT="/absolute/path/to/SDK" CUDA_ROOT="/path/to/cuda"
make run SDK_ROOT="/absolute/path/to/SDK" CUDA_ROOT="/path/to/cuda" GPU_INDEX=0
```

`GPU_INDEX` indexes the licensed-GPU list; it is not a CUDA device ordinal.
For direct execution, use:

```bash
export LD_LIBRARY_PATH="/absolute/path/to/SDK/lib:/path/to/cuda/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
./sample -g 0 data/matrix_real.mtx data/rhs_real_1.vec data/rhs_real_2.vec data/matrix_real_b.mtx data/rhs_real_b_1.vec data/rhs_real_b_2.vec
```

Replace both library paths with actual locations. `make run` sets these search
paths automatically from `SDK_ROOT` and `CUDA_LIB_DIR`.

The included `data/` files provide a small test input. `sample.c` contains the
API workflow; the local `sample_common.h` implements file reading, uploads,
phase timing, and original-equation residual checks.

Custom matrix inputs must have identical sparse patterns, not only equal order and stored-entry count. The sample reports analysis once, numerical factorization for the first matrix, refactorization for the second, and four individual solves.

The output reports phase times in milliseconds and the relative infinity-norm
residual. Review the residual independently of process/API success.
See the parent README for prerequisites, input formats, and license setup.
Run `make clean` to remove the executable.
