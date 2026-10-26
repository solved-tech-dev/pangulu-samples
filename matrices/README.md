# Shared Sample Inputs

These fixed text files are shared by the samples. They contain input data only;
keep binaries, build products, solver results, logs, packages, and licenses out
of this directory. See [PATTERN_FORMAT.md](PATTERN_FORMAT.md) for storage rules.

| Matrix | Order | Stored entries | File field | Solve precision | Used by |
| --- | --- | --- | --- | --- | --- |
| [matrix_real.mtx](matrix_real.mtx) | 19 x 19 | 83 | Real symmetric | Single or double | 01, 02, 05, 06, 07, 08, 09 |
| [matrix_real_b.mtx](matrix_real_b.mtx) | 19 x 19 | 83 | Real symmetric | Double | 09, second matrix |
| [matrix_complex.mtx](matrix_complex.mtx) | 48 x 48 | 224 | Complex Hermitian | Single or double | 03, 04 |

Stored-entry counts refer to records in the file. The reader expands declared
symmetry and retains repeated coordinates, so the in-memory nonzero count can
be larger. Real matrices A and B have identical sparse patterns but different
values.

| RHS file | Values | Used with |
| --- | --- | --- |
| [rhs_real_1.vec](rhs_real_1.vec) | 19 real values | `matrix_real.mtx`; default real RHS |
| [rhs_real_2.vec](rhs_real_2.vec) | 19 real values | `matrix_real.mtx`; second RHS in 08 and 09 |
| [rhs_real_b_1.vec](rhs_real_b_1.vec) | 19 real values | `matrix_real_b.mtx`; first RHS in 09 |
| [rhs_real_b_2.vec](rhs_real_b_2.vec) | 19 real values | `matrix_real_b.mtx`; second RHS in 09 |
| [rhs_complex_1.vec](rhs_complex_1.vec) | 48 real/imaginary pairs | `matrix_complex.mtx`; complex RHS |

Inputs contain no random seed, reference-solution field, or size header in the
RHS files. Samples read the RHS directly and verify the original equation.
Single-precision samples convert these inputs for computation while retaining
the original values for verification.

To supply your own matrix and RHS, keep the argument order documented in the
sample README. From the repository root, for example:

```bash
make -C 01_double run RUN_ARGS='/absolute/path/matrix.mtx /absolute/path/rhs.vec'
```

Relative paths are interpreted from the sample directory. Refactorization
requires exactly matching compressed pointers and indices for both matrices.
