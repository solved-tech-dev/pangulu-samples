# PanguLU Samples

## Overview

PanguLU solves sparse linear systems `A*x = b` with direct LU factorization.
The C samples cover real and complex arithmetic, single and double precision,
CSC and CSR input, host-memory matrix input, reordering, multiple right-hand
sides (RHS), and refactorization.

The product algorithm implementation, library packages, runtime licenses, and
prebuilt executables are not included. Download software and purchase licenses
from the [Solved Technologies (解得科技) official website](https://store.solved-tech.com/).

## Repository Layout

```text
pangulu-samples/
├── README.md
├── Makefile                   # Build or clean all samples
├── common.mk                  # Shared build, run, and clean rules
├── common/
│   └── sample_common.h        # Input, transfer, timing, and verification helpers
├── libraries/
│   ├── README.md              # Package and license setup
│   └── <package-name>/        # User-supplied complete package; not committed
│       ├── include/
│       ├── lib/
│       ├── license.key
│       └── ...                # Preserve all package runtime files
├── matrices/
│   ├── README.md              # Shared input inventory
│   ├── PATTERN_FORMAT.md      # Matrix and RHS formats
│   └── ...                    # Shared matrix and RHS files
├── docs/
│   ├── PRODUCT_GUIDE.md       # C API, configuration, and resource ownership
│   └── TROUBLESHOOTING.md     # Common problems and recovery steps
├── 01_double/
├── 02_float/
├── 03_double_complex/
├── 04_float_complex/
├── 05_csr/
├── 06_cpu_matrix_input/
├── 07_l0_reordering/
├── 08_multi_rhs/
└── 09_refactor/
```

Each sample contains `sample.c`, a short `Makefile`, and `README.md`.
All samples use `common.mk`, `common/sample_common.h`, and shared inputs in
`matrices/`. Configuration is set through C API calls in `sample.c`; these
samples do not need `config.json`.

## Quick Start

### Requirements

- Linux.
- The GPU and the toolkit/runtime required by the selected package.
- A C compiler and GNU Make.
- A compatible PanguLU 26.43 package with the single-vector and multiple-vector
  interfaces described in the [product guide](docs/PRODUCT_GUIDE.md).
- A valid license matching the package and target device.

Follow the selected package's README for supported GPU models, architectures,
compiler requirements, and toolkit/runtime versions. Keep its runtime files
and architecture subdirectories intact.

### Library Package and License

1. Download the package for your target GPU and purchase its license at
   [Solved Technologies](https://store.solved-tech.com/).
2. Extract the complete package into `libraries/<package-name>/`.
3. Place its license at `libraries/<package-name>/license.key`.

```text
libraries/
└── <package-name>/
    ├── include/
    ├── lib/
    ├── license.key
    └── ...
```

Replace `<package-name>` with the actual extracted directory name. Package
files and keys are ignored by Git. The repository's `LICENSE` covers repository
source and does not replace the product's runtime license.

When exactly one package containing `include/pangulu.h` is present under
`libraries/`, it is selected automatically. With multiple packages or a package
elsewhere, set `LIBRARY_ROOT` as shown below. This argument selects a directory.
`make run` uses the selected package's `license.key` by default. See
[libraries/README.md](libraries/README.md) for license overrides.

### Build and Run

From the repository root, after package and environment setup:

```bash
make -C 01_double
make -C 01_double run
```

Or work from the sample directory:

```bash
cd 01_double
make
make run
make clean
```

Build or clean all samples from the repository root:

```bash
make
make clean
```

Select a package explicitly with an absolute path:

```bash
make -C 01_double LIBRARY_ROOT=/path/to/package
make -C 01_double run LIBRARY_ROOT=/path/to/package
```

Use the same package path for both commands. `make run` sets the selected
package and runtime library search paths for the launched program. If the
required development environment uses a nondefault installation path, follow
[the environment setup instructions](docs/TROUBLESHOOTING.md#toolkit-and-runtime-paths).

`GPU_INDEX` defaults to `0`, the first entry in the ordered licensed-GPU list;
it is not a physical device ordinal. Select another licensed entry with
`make -C 01_double run GPU_INDEX=1`. Run `make clean` before changing source,
package, compiler settings, or the toolkit/runtime environment.

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

*Single-precision samples convert input to single precision for the solve and
validate against the original double-precision system. Host-memory matrix input
still uses GPU computation, RHS, and solution buffers. L1 reordering enables
MC64 matching/scaling; L0 omits MC64 matching. Reordering and factor reuse are
separate settings. These direct solves do not use an iterative method or
preconditioner.

### Sample Input

Shared matrix and RHS files are in [matrices/](matrices/README.md). The
[format description](matrices/PATTERN_FORMAT.md) explains symmetry, indexing,
and RHS layout. Each sample README links to its default files and argument order.

- Matrices use Matrix Market coordinate format with one-based file indices.
  The reader expands declared symmetry and builds zero-based CSC or CSR arrays.
- Compressed pointers are signed 64-bit integers; indices are signed 32-bit
  integers. The matrix must be square with positive order at most `INT32_MAX`.
- Real samples accept real, integer, or pattern input; pattern values are `1`.
  Complex samples require complex input and store interleaved real/imaginary
  components. Solve precision is listed above.
- RHS files contain `n` real values or `n` real/imaginary pairs without a header.
  Sample 08 packs columns in column-major order with leading dimension `n`.
- Sample 09 requires identical sparse patterns, including compressed pointers
  and indices, for both matrices. Only numerical values may change.
- Inputs are fixed text files with no random seed or reference-solution field.
  RHS values are read from files rather than generated randomly. Configuration
  is set in `sample.c`; no JSON file is read.

Replace inputs without editing build rules. From the repository root:

```bash
make -C 01_double run RUN_ARGS='/absolute/path/matrix.mtx /absolute/path/rhs.vec'
```

Preserve the argument order from the individual README. Relative paths in
`RUN_ARGS` are resolved from the sample directory used by `make -C`.

## Output and Verification

Samples print input paths, matrix order and expanded nonzero count, and library
phase times in milliseconds: `Analysis time`, `Numeric factorisation time`,
and `Solving time`. Sample 09 also prints `Refactorisation time`. Samples 08
and 09 label each RHS result. Timing excludes input parsing, application data
transfers, and host residual checks. A direct solve has no iteration count or
iterative convergence field.

API success alone does not establish numerical accuracy. Each solution is
checked independently on the host against the original system `r = b - A*x`:

```text
relative residual = ||r||_inf / ||b||_inf
```

The RHS infinity norm is printed separately. Complex norms use the maximum
complex magnitude. For zero RHS, the sample checks the absolute infinity norm
`||r||_inf` and notes that the relative residual is undefined. Accumulation uses
`long double`; single-precision solves use the original double-precision input
for verification.

`verification: PASS` means the original-system residual is finite and no greater
than `1e-10` for double-precision solves or `1e-5` for single-precision solves.
For zero RHS, the same limit applies to the absolute residual. A non-finite or
excessive residual prints `FAIL` and makes the program fail. These sample
thresholds are examples; assess the accuracy required by your own application.

Program exit code `0` means the execution and all required residual checks
succeeded. Code `1` indicates execution or verification failure; code `2`
indicates an argument or initial input error. `make run` reports a failed program
through Make's own nonzero exit code. See the
[product guide](docs/PRODUCT_GUIDE.md#results-and-resource-lifetime) for API and
resource-lifetime details.

## Troubleshooting

Start with [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) for missing headers,
shared or indirect runtime libraries, license errors, GPU/package mismatch,
environment mismatch, configuration errors, input paths, failed solves or
residual checks, and stale builds after source changes.
