# PanguLU Samples

## Overview

PanguLU solves sparse linear systems `A*x = b` using direct LU factorization.
This repository provides complete C samples for real and complex arithmetic,
single and double precision, CSC and CSR matrix input, host-memory matrix
input, reordering, multiple right-hand sides (RHS), and refactorization.
No iterative method or preconditioner is configured by these samples.

The library package, license, product algorithm implementation, and prebuilt
executables are not included. Download software and purchase licenses from the
[Solved Technologies (解得科技) official website](https://store.solved-tech.com/).

## Repository Layout

```text
pangulu-samples/
├── README.md                  # Repository setup and usage
├── Makefile                   # Build or clean all samples
├── LICENSE                    # Repository source license
├── 01_double/                 # Double-precision real solve
├── 02_float/                  # Single-precision real solve
├── 03_double_complex/         # Double-precision complex solve
├── 04_float_complex/          # Single-precision complex solve
├── 05_csr/                    # CSR matrix input
├── 06_cpu_matrix_input/       # Host-memory matrix input
├── 07_l0_reordering/          # L0 reordering
├── 08_multi_rhs/              # Multiple RHS in one solve
└── 09_refactor/               # Refactorization and factor reuse
```

Each sample directory contains `sample.c`, `Makefile`, `README.md`,
`sample_common.h`, and `data/`. The helper header handles input reading,
data transfer, timing, and independent residual calculation. Inputs and build
rules are local to each sample, so a sample directory can also be copied
elsewhere and built with an explicit package path.

The following package location is recommended for setup. Create `libraries/`
locally; the downloaded package and its license must not be committed:

```text
pangulu-samples/
└── libraries/
    └── <package-name>/
        ├── include/
        ├── lib/
        ├── license.key
        └── ...               # Keep all other package files
```

## Quick Start

### Requirements

- Linux.
- The GPU and the toolkit/runtime required by the selected package.
- A C compiler and GNU Make.
- A PanguLU package matching the target system and GPU, and a valid license
  for that package and device.

Follow the selected package's README for supported GPU models, architectures,
and toolkit/runtime versions. Check its `BUILD_INFO.json` and `RELEASE_NOTES.md`
when supplied. Multiple-RHS support requires PanguLU 26.43 or a later compatible
package exporting `pangulu_multiple_vector_create`.

### Library Package and License

1. Download the package for your target GPU and purchase the matching license
   at [Solved Technologies](https://store.solved-tech.com/).
2. From the repository root, create `libraries/` with `mkdir -p libraries`,
   then extract the complete package into `libraries/<package-name>/`.
3. Place the license at `libraries/<package-name>/license.key`.
4. Keep all package architecture subdirectories and runtime files intact.
   Do not edit the license file.

Replace `<package-name>` with the actual extracted directory name, then run
these commands from the repository root:

```bash
export PANGULU_SDK="$(pwd)/libraries/<package-name>"
export PANGULU_LICENSE_PATHS="$PANGULU_SDK/license.key"
```

Check the package layout before building:

```bash
test -f "$PANGULU_SDK/include/pangulu.h"
test -f "$PANGULU_LICENSE_PATHS"
```

Set these environment variables again in each new terminal or batch job.
Runtime license authorization is local and does not require internet access.
The repository's `LICENSE` covers repository source; it does not replace the
runtime license required by the downloaded software.

### Build and Run

After setting up the package and environment above, run from the repository root:

```bash
make -C 01_double
make -C 01_double run
make -C 01_double clean
```

Or run from the sample directory:

```bash
cd 01_double
make
make run
make clean
```

From the repository root, build or clean all samples with:

```bash
make
make clean
```

When the package is elsewhere, set its absolute path explicitly:

```bash
export PANGULU_LICENSE_PATHS="/path/to/package/license.key"
make -C 01_double SDK_ROOT=/path/to/package
make -C 01_double run SDK_ROOT=/path/to/package
```

`SDK_ROOT` is the package-directory argument supported by the current
Makefiles; it selects a package path. `PANGULU_SDK` supplies the same path
through the environment. `make run` sets the package and configured runtime
library search paths for the launched program, including indirect dependencies.
Follow the package's environment setup instructions if its required toolkit
is installed outside the build defaults.

`GPU_INDEX` defaults to `0`, the first entry in the ordered licensed-GPU list.
It is a license-list index, not a physical device ordinal. Select another
licensed entry with, for example, `make -C 01_double run GPU_INDEX=1`.
Run `make clean` before changing source, the package, compiler options, or
the toolkit/runtime environment.

## Samples

### Samples List

| Sample name | Method | Matrix format | Precision |
| --- | --- | --- | --- |
| [01_double](01_double/README.md) | Direct LU, L1 reordering | CSC, device memory | Real double |
| [02_float](02_float/README.md) | Direct LU, L1 reordering | CSC, device memory | Real single* |
| [03_double_complex](03_double_complex/README.md) | Direct LU, L1 reordering | CSC, device memory | Complex double |
| [04_float_complex](04_float_complex/README.md) | Direct LU, L1 reordering | CSC, device memory | Complex single* |
| [05_csr](05_csr/README.md) | Direct LU, L1 reordering | CSR, device memory | Real double |
| [06_cpu_matrix_input](06_cpu_matrix_input/README.md) | Direct LU, L1 reordering | CSC, host memory* | Real double |
| [07_l0_reordering](07_l0_reordering/README.md) | Direct LU, L0 reordering | CSC, device memory | Real double |
| [08_multi_rhs](08_multi_rhs/README.md) | Direct LU, shared factors for multiple RHS | CSC, device memory | Real double |
| [09_refactor](09_refactor/README.md) | Direct LU, refactorization with pattern reuse | CSC, device memory | Real double |

*Single-precision samples convert double-precision input to single precision
for the solve and validate against the original double-precision system.
Host-memory matrix input still uses GPU factorization, RHS, and solution
buffers. L1 reordering enables MC64 matching and scaling; L0 omits MC64 matching.
Reordering levels and factor-reuse levels are separate configuration options.

### Sample Input

Each sample's `data/` directory contains its default matrix and RHS files.
The [sample READMEs](#samples-list) link to their inputs and show the required
argument order. These samples do not read `config.json`: configuration is
set in `sample.c` using the C API, and input paths are command-line arguments.

- **Matrix:** Matrix Market coordinate (`.mtx`) format with one-based file
  indices. Matrices must be square and have positive order, at most `INT32_MAX`.
  Real samples accept real, integer, or pattern fields; complex samples require
  a complex field. Pattern entries have value `1`. Declared symmetry is expanded
  before solving and verification.
- **In-memory layout:** the reader produces zero-based CSC arrays, or CSR in
  sample 05, with signed 64-bit compressed pointers and signed 32-bit indices.
  Real values are scalar elements; complex values use interleaved real and
  imaginary components. See each row above for solve precision.
- **RHS:** plain-text `.vec` files without a size header. Real input contains
  `n` values; complex input contains `n` real/imaginary pairs. Sample 08 packs
  its RHS columns in column-major order with leading dimension `n`.
- **Refactorization:** sample 09 requires the same sparse pattern in both
  matrices, including compressed pointers and indices, with only values changing.
  Matching dimensions and entry counts alone are insufficient.
- **Reproducibility:** the supplied inputs are fixed text files with no random
  seed or reference-solution field. Samples read the RHS from these files;
  they do not generate it randomly or require a stored known solution.

Replace inputs without editing the Makefile by setting `RUN_ARGS`. For example,
from the repository root, after setting up the package:

```bash
make -C 01_double run RUN_ARGS='/absolute/path/matrix.mtx /absolute/path/rhs.vec'
```

Keep the argument order shown in the individual README. Paths in `RUN_ARGS`
are resolved from the sample directory used by `make -C`.

## Output and Verification

Samples print the selected operation, matrix order and expanded nonzero count,
input paths, and library phase times in milliseconds. Common timing fields are
`Analysis time`, `Numeric factorisation time`, and `Solving time`. Sample 09
also prints `Refactorisation time`; samples 08 and 09 identify each RHS result.
These are direct solves, so there is no iteration count or convergence field.
Timings cover the library calls, excluding input parsing, application transfers,
and residual checks.

After downloading the solution, the sample independently computes the original
linear-system residual `r = b - A*x` on the host. The reported value is:

```text
relative residual = ||r||_inf / ||b||_inf
```

For complex vectors, the infinity norm is the maximum complex magnitude.
For zero RHS, the relative residual is undefined and the sample reports
`||r||_inf` with an explicit note instead. Residual accumulation uses
`long double`; single-precision solves are checked against the original input.
Non-finite values are reported as invalid.

An API success status does not establish application accuracy. Choose an
acceptable tolerance for your system and inspect every reported residual.
The current samples do not print `PASS` or enforce a residual threshold:
exit code `0` means the execution and reporting workflow completed, and can
still accompany a large or non-finite residual. Exit code `1` indicates an
execution/allocation failure; `2` indicates an argument or initial input error.
`make run` propagates a failed recipe through Make's own nonzero exit code.
A numerical **PASS** should be accepted only after every original-system
residual is finite and within the application's required tolerance.

## Troubleshooting

| Problem | What to check |
| --- | --- |
| Header not found | Point `PANGULU_SDK` or `SDK_ROOT` to the package containing `include/pangulu.h`. Confirm the selected package's required development environment is installed. |
| Shared library not found | Keep the package's `lib/` directory intact and use `make run`. For direct execution, set `LD_LIBRARY_PATH` to the package and required runtime library directories. |
| Indirect runtime library not found | Restore the complete package, including architecture subdirectories, and follow its README to set runtime search paths. A linker search path alone does not resolve every runtime dependency. |
| License missing, expired, or device mismatch | Check `PANGULU_LICENSE_PATHS`, validity, package compatibility, and the licensed device. Obtain the correct license from [Solved Technologies](https://store.solved-tech.com/). |
| GPU and package mismatch | Download the package for the actual target GPU and check its supported architecture list. |
| Build/runtime environment mismatch | Follow the selected package's compiler and toolkit/runtime requirements; avoid mixing libraries from different package versions. |
| Invalid configuration field | These samples use C API configuration, not JSON. Check configuration keys and values against the package's `include/pangulu.h`, then clean and rebuild. |
| Cannot read matrix or RHS | Check paths from the sample directory, matrix field and dimensions, and RHS length. Complex samples require real/imaginary pairs. |
| Solve fails or residual is too large | Inspect the reported phase status, singularity and scaling, precision, RHS, and original-system residual. A direct solve has no iterative convergence flag; API completion alone is insufficient. |
| Refactorization fails | Use matrices with identical compressed pointers and indices; keep the sparsity pattern unchanged. |
| Source/package changed but results are stale | Run `make clean`, then build and run again in the same configured environment. |

For C API declarations and available configuration values, use the selected
package's `include/pangulu.h` and product guide. Each linked `sample.c` shows
the complete object-creation, execution, verification, and cleanup workflow.
