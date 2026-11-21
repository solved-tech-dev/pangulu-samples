# ND

Input: [`nd_separator.mtx`](../matrices/ordering/nd_separator.mtx).

The 9-by-9 graph consists of two four-vertex cliques joined through vertex 5.
Removing that vertex separates the two cliques. Nested dissection uses
separators of this kind and puts them after the subgraphs they separate.

From the repository root, run with a PanguLU 26.48 package:

```sh
make -C 10_reordering_nd run LIBRARY_ROOT=/path/to/package
```

The sample constructs `b = A * 1`, solves the system, and checks the relative L2
residual against the original matrix. See the [ordering API
guide](../docs/PRODUCT_GUIDE.md#ordering-selection).