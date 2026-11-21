# BTF-COLAMD

Input: [`btf_colamd_arrowhead_blocks.mtx`](../matrices/ordering/btf_colamd_arrowhead_blocks.mtx).

The 8-by-8 matrix has an irregular five-vertex arrowhead block and a
three-vertex cycle block, with one-way coupling. BTF-COLAMD finds the block
triangular structure and applies a column approximate minimum degree ordering
inside the blocks.

From the repository root, run with a PanguLU 26.48 package:

```sh
make -C 14_reordering_btf_colamd run LIBRARY_ROOT=/path/to/package
```

The sample constructs `b = A * 1`, solves the system, and checks the relative L2
residual against the original matrix. See the [ordering API
guide](../docs/PRODUCT_GUIDE.md#ordering-selection).