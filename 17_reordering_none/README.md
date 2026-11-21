# No reordering

Input: [`none_tridiagonal.mtx`](../matrices/ordering/none_tridiagonal.mtx).

The 5-by-5 matrix is already tridiagonal in its original row and column order.
Selecting NONE keeps that order.

From the repository root, run with a PanguLU 26.48 package:

```sh
make -C 17_reordering_none run LIBRARY_ROOT=/path/to/package
```

The sample constructs `b = A * 1`, solves the system, and checks the relative L2
residual against the original matrix. See the [ordering API
guide](../docs/PRODUCT_GUIDE.md#ordering-selection).