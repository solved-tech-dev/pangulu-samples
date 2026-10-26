# Host-Memory Matrix Input

Pass a double-precision CSC matrix in host memory. The library handles its internal upload; computation, RHS data, and the solution still use the GPU. This does not enable hybrid execution.

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

Host matrix input does not move factorization or solve to the CPU.

The output reports phase times in milliseconds and the relative infinity-norm
residual. Review the residual independently of process/API success.
See the parent README for prerequisites, input formats, and license setup.
Run `make clean` to remove the executable.
