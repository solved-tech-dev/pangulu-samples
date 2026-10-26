# Single-Precision Real Solve

Convert real input to single precision, solve with a GPU-resident CSC matrix and L1 reordering, and validate against the original double-precision input.

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
./sample -g 0 data/matrix_real.mtx data/rhs_real_1.vec
```

Replace both library paths with actual locations. `make run` sets these search
paths automatically from `SDK_ROOT` and `CUDA_LIB_DIR`.

The included `data/` files provide a small test input. `sample.c` contains the
API workflow; the local `sample_common.h` implements file reading, uploads,
phase timing, and original-equation residual checks.

The output reports phase times in milliseconds and the relative infinity-norm
residual. Review the residual independently of process/API success.
See the parent README for prerequisites, input formats, and license setup.
Run `make clean` to remove the executable.
