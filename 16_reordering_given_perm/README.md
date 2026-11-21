# User-supplied permutations

Input: [`given_perm_shuffled_triangular.mtx`](../matrices/ordering/given_perm_shuffled_triangular.mtx).

The 4-by-4 matrix was built by shuffling the rows and columns of an
upper-triangular matrix with different permutations. The zero-based arrays in
`sample.c` map new indices to original indices and recover the upper-triangular
structure.

The first ANALYSIS call supplies the matrix shape and returns
`PANGULU_STATUS_REORDERING_PERM_NOT_SET`. The sample then calls
`pangulu_reordering_perm_set` with both arrays before retrying ANALYSIS.

From the repository root, run with a PanguLU 26.48 package:

```sh
make -C 16_reordering_given_perm run LIBRARY_ROOT=/path/to/package
```

The sample constructs `b = A * 1`, solves the system, and checks the relative L2
residual against the original matrix. See the [ordering API
guide](../docs/PRODUCT_GUIDE.md#ordering-selection).