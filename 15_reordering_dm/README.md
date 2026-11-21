# DM

Input: [`dm_shuffled_rows.mtx`](../matrices/ordering/dm_shuffled_rows.mtx).

The 6-by-6 input starts from a two-block triangular system and shuffles only the
rows. Its two diagonal blocks are no longer apparent from the input indices.
Dulmage–Mendelsohn uses bipartite matching and independent row and column orders
to expose the block structure.

From the repository root, run with a PanguLU 26.48 package:

```sh
make -C 15_reordering_dm run LIBRARY_ROOT=/path/to/package
```

The sample constructs `b = A * 1`, solves the system, and checks the relative L2
residual against the original matrix. See the [ordering API
guide](../docs/PRODUCT_GUIDE.md#ordering-selection).