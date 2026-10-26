# Troubleshooting

Run commands below from the repository root unless stated otherwise. Follow the
selected package's README for the GPU and the toolkit/runtime required by the
selected package. Download software and licenses from
[Solved Technologies](https://store.solved-tech.com/).

## Package Selection and Missing Headers

Place the full package in `libraries/<package-name>/`. Automatic selection
requires exactly one directory containing `include/pangulu.h`. If there is no
package or more than one package, supply an absolute path:

```bash
make -C 01_double LIBRARY_ROOT=/path/to/package
```

The path must contain `include/` and `lib/`, rather than point to either of
those subdirectories. Check for `include/pangulu.h`, `lib/libpangulu.so`, and
`lib/libpangulu_platform.so`. Do not mix headers from one package with libraries
from another. If a vector-constructor declaration differs from the samples,
choose a package with the interfaces described in the
[product guide](PRODUCT_GUIDE.md#scope-and-package-compatibility).

## Toolkit and Runtime Paths

Install and initialize the development environment required by the selected
package. The common rules accept generic installation-path overrides:

```bash
make -C 01_double LIBRARY_ROOT=/path/to/package TOOLKIT_ROOT=/path/to/toolkit
make -C 01_double run LIBRARY_ROOT=/path/to/package TOOLKIT_ROOT=/path/to/toolkit
```

If runtime libraries are outside the detected `lib64/` or `lib/` location,
supply `TOOLKIT_LIB_DIR=/path/to/runtime/lib` to both commands. These arguments
specify filesystem paths. Compiler and toolkit/runtime versions must match
the package's requirements. Unsupported host or GPU architectures require the
matching package; changing a path alone cannot make them compatible.

## Shared Libraries and Indirect Dependencies

Use `make run` to supply the configured package and runtime search paths.
Keep the complete package, including architecture subdirectories and supporting
runtime files. Restoring only `libpangulu.so` is insufficient when it needs
other shared libraries.

For direct execution, initialize the search path as required by the package:

```bash
export LD_LIBRARY_PATH="/path/to/package/lib:/path/to/runtime/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
```

Retain additional architecture or runtime paths specified by the package's
README. Link-time `-L` options do not resolve every indirect runtime dependency.

## License Missing, Expired, or Device Mismatch

By default, `make run` uses the selected package's `license.key`. Check that it
exists, is readable and valid, and matches the actual package and target device.
Do not edit a key file. Obtain a matching license from the official store.

An existing `PANGULU_LICENSE_PATHS` environment value overrides the default.
Clear a stale override with `unset PANGULU_LICENSE_PATHS`, or pass the intended
absolute license path explicitly. `GPU_INDEX` indexes the ordered licensed-GPU
list, not the physical device list. See [license setup](../libraries/README.md).

## GPU or Build Environment Mismatch

Check the package's supported GPU architecture, Linux runtime, compiler, and
required toolkit/runtime versions. Use the correct full package rather than
combining files from different installations. The samples require Linux;
a successful source inspection on another operating system does not establish
runtime compatibility.

## Configuration Errors

Samples configure the library through C API calls in `sample.c`; there is no
JSON configuration file to edit. Check option/value pairs against the selected
package's `include/pangulu.h`. Reordering and factor reuse are separate options.
Set them before the first dependent execution phase, then clean and rebuild.

## Matrix or RHS Path Errors

Default inputs live in `matrices/` and are selected by each sample's Makefile.
With `make -C`, relative input paths are interpreted from the sample directory.
Use absolute paths for custom inputs. Preserve the argument order from the
sample README, matrix field and dimensions, and RHS length. Complex input needs
real/imaginary pairs. See [input formats](../matrices/PATTERN_FORMAT.md).

## Solve Failure or Verification Failure

These are direct LU solves, so they do not have an iterative convergence flag.
Check the failed phase status first. Confirm that the matrix is nonsingular,
the RHS matches the matrix, and the selected precision meets the problem's
accuracy needs. Inspect every original-system residual; API success alone is
insufficient. Non-finite values or residuals above the documented tolerance
produce `FAIL` and a nonzero program exit code.

For refactorization, both matrices must have identical compressed pointers and
indices, including entry order. Only values may change. A dimension/nonzero
count match alone is insufficient.

## Stale Build After Changes

Run `make clean` before changing package, compiler options, or the required
toolkit/runtime environment, and after editing source. Then rebuild:

```bash
make clean
make LIBRARY_ROOT=/path/to/package
make -C 01_double run LIBRARY_ROOT=/path/to/package
```

Root `make clean` removes sample executables and does not remove packages,
licenses, or inputs. Make may report its own nonzero exit code when the sample
program fails; see the last program error and residual output for the cause.
