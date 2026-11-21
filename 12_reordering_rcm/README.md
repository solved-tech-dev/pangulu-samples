# RCM

Input: [`rcm_scrambled_path.mtx`](../matrices/ordering/rcm_scrambled_path.mtx).

The 8-by-8 graph is a path numbered 1, 8, 2, 7, 3, 6, 4, 5. Its input bandwidth
is 7. Traversing in path order (in either direction) reduces the bandwidth to 1.
Reverse Cuthill–McKee is a bandwidth-reduction ordering.

From the repository root, run with a PanguLU 26.48 package:

```sh
make -C 12_reordering_rcm run LIBRARY_ROOT=/path/to/package
```

The sample constructs `b = A * 1`, solves the system, and checks the relative L2
residual against the original matrix. See the [ordering API
guide](../docs/PRODUCT_GUIDE.md#ordering-selection).