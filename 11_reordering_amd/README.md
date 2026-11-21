# AMD

Input: [`amd_arrowhead.mtx`](../matrices/ordering/amd_arrowhead.mtx).

The 7-by-7 arrowhead has a hub at vertex 1 and six leaves. Eliminating the hub
first would connect all six leaves. Approximate minimum degree instead favors
the low-degree leaves to limit fill.

From the repository root, run with a PanguLU 26.48 package:

```sh
make -C 11_reordering_amd run LIBRARY_ROOT=/path/to/package
```

The sample constructs `b = A * 1`, solves the system, and checks the relative L2
residual against the original matrix. See the [ordering API
guide](../docs/PRODUCT_GUIDE.md#ordering-selection).