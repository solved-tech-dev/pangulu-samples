# Library Package and License

Download the package for your target GPU and purchase a matching license from
the [Solved Technologies (解得科技) official website](https://store.solved-tech.com/).
Follow the package's README for the GPU and the toolkit/runtime required by
the selected package.

Extract the complete package here, then place its license beside `include/`
and `lib/`:

```text
libraries/
├── README.md
└── <package-name>/
    ├── include/
    │   └── pangulu.h
    ├── lib/
    ├── license.key
    └── ...
```

Keep all architecture subdirectories, runtime files, and other package content.
Do not edit the license file. Packages, license keys, and binaries are excluded
from Git by the repository's ignore rules.

If exactly one subdirectory contains `include/pangulu.h`, the build selects it
automatically. For multiple packages, or a package stored elsewhere, use an
absolute path from the repository root:

```bash
make -C 01_double LIBRARY_ROOT=/path/to/package
make -C 01_double run LIBRARY_ROOT=/path/to/package
```

`LIBRARY_ROOT` only specifies the package directory. Use the same directory for
building and running, and run `make clean` before switching packages.
The samples use the five-argument `pangulu_vector_create` interface and the
separate `pangulu_multiple_vector_create` interface; choose a compatible package
as described in the [product guide](../docs/PRODUCT_GUIDE.md).

`make run` sets `PANGULU_LICENSE_PATHS` to the selected package's `license.key`
when the variable is unset. An existing environment setting or command-line
assignment overrides that default. To use a different valid license:

```bash
make -C 01_double run PANGULU_LICENSE_PATHS=/absolute/path/license.key
```

Use `unset PANGULU_LICENSE_PATHS` to return to the package-local default in the
current terminal. License authorization is local and requires no internet
connection at runtime. The source-code `LICENSE` at the repository root does
not grant runtime authorization for the downloaded software.
