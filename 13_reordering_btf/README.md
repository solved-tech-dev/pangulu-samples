# BTF

Input: [`btf_two_blocks.mtx`](../matrices/ordering/btf_two_blocks.mtx).

The 6-by-6 matrix has directed 3-cycles on interleaved index sets {1, 3, 5}
and {2, 4, 6}, with coupling only from the first group to the second. Block triangular form exposes two
diagonal blocks of order 3.

From the repository root, run with a PanguLU 26.48 package:

```sh
make -C 13_reordering_btf run LIBRARY_ROOT=/path/to/package
```

The sample constructs `b = A * 1`, solves the system, and checks the relative L2
residual against the original matrix. See the [ordering API
guide](../docs/PRODUCT_GUIDE.md#ordering-selection).