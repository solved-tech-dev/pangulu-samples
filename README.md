# PanguLU Samples

Nine independent CUDA C samples based on the PanguLU 26.43 delivery examples.
Each directory contains `sample.c`, its local `sample_common.h`, a Makefile,
a short README, and the input data needed for its default run. No prebuilt
executables, SDK libraries, or license files are included.

| Directory | Demonstration |
| --- | --- |
| [01_double](01_double/README.md) | Double-Precision Real Solve |
| [02_float](02_float/README.md) | Single-Precision Real Solve |
| [03_dcomplex](03_dcomplex/README.md) | Double-Precision Complex Solve |
| [04_fcomplex](04_fcomplex/README.md) | Single-Precision Complex Solve |
| [05_csr](05_csr/README.md) | CSR Matrix Input |
| [06_cpu](06_cpu/README.md) | Host-Memory Matrix Input |
| [07_l0](07_l0/README.md) | L0 Reordering |
| [08_multi_rhs](08_multi_rhs/README.md) | Multiple RHS in One Solve Call |
| [09_refactor](09_refactor/README.md) | Refactorization and Factor Reuse |

## Prerequisites

- A PanguLU SDK matching the target Linux system and GPU. Check its
  `BUILD_INFO.json` and `RELEASE_NOTES.md`.
- A C compiler, GNU Make, and a compatible CUDA development toolkit, including
  headers and `libcudart.so`.
- A valid original PanguLU license for the selected GPU. Runtime authorization
  is local and does not require internet access. Do not edit the license file.

## Quick start

Keep this directory anywhere and point to the extracted SDK explicitly:

```bash
export PANGULU_SDK="/absolute/path/to/SDK"
export PANGULU_LICENSE_PATHS="/absolute/path/to/license.key"
export LD_LIBRARY_PATH="$PANGULU_SDK/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd pangulu_samples/01_double
make SDK_ROOT="$PANGULU_SDK" CUDA_ROOT="/path/to/cuda"
make run SDK_ROOT="$PANGULU_SDK" CUDA_ROOT="/path/to/cuda" GPU_INDEX=0
```

Replace the paths with actual locations. These environment settings apply to
the current terminal and its child processes. A new terminal or batch job
needs its own settings. `GPU_INDEX` is a zero-based index into the ordered
licensed-GPU list, not a CUDA device ordinal.

Alternatively, place this directory at `SDK/pangulu_samples/`. Each sample then
finds the containing SDK by default, unless `PANGULU_SDK` or `SDK_ROOT` overrides
it. An individual sample directory can also be copied elsewhere and built
with an explicit `SDK_ROOT`; it does not depend on sibling samples.

To compile all nine samples from this directory:

```bash
make SDK_ROOT="/absolute/path/to/SDK" CUDA_ROOT="/path/to/cuda"
```

Each Makefile supports `CC`, `CFLAGS`, `CPPFLAGS`, `LDFLAGS`, `LDLIBS`,
`SDK_ROOT`, `CUDA_ROOT`, and `CUDA_LIB_DIR`. Override `CUDA_LIB_DIR` if CUDA
libraries are not in the toolkit's `lib64/` or `lib/`. Use `make clean` before
changing the SDK, toolkit, or build flags. Run targets use the included data
by default; custom arguments can be supplied as `RUN_ARGS='...'`. `make run`
adds the SDK and CUDA library directories to `LD_LIBRARY_PATH` for the launched
process, including indirect runtime dependencies. For direct execution, set
both library directories in the launch environment as shown in each README.

## Data and output

Matrices use Matrix Market coordinate files. The reader expands declared
symmetry and converts one-based file indices to zero-based CSC or CSR arrays.
Real RHS files contain `n` whitespace-separated values; complex RHS files
contain `n` real/imaginary pairs. RHS files have no size header.

The samples report phase times in milliseconds and validate the original
equation independently on the host. The printed residual is
`||A*x-b||_inf / ||b||_inf`, using complex magnitudes for complex vectors.
A zero RHS instead produces an absolute residual with an explicit note.
Non-finite results are reported rather than replaced with zero. Process/API
success alone does not establish numerical accuracy; review every RHS result.

The matrix may be in host memory in sample 06, but RHS and solution buffers
are GPU-resident in every sample. Sample 08 uses the new multiple-RHS API;
the other single-vector calls retain the five-argument compatible interface.
Timing covers the selected library phases, not file reading, application
transfers, or residual checks. The samples do not rebuild or modify the SDK.
