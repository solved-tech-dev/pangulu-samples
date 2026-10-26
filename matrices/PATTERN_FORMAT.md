# Matrix and RHS File Formats

## Matrix Market Coordinate Input

A real matrix starts with a header such as:

```text
%%MatrixMarket matrix coordinate real general
% Optional comment
3 3 4
1 1 4.0
1 2 1.0
2 2 5.0
3 3 6.0
```

The size line is `rows columns stored_entries`; each real entry is
`row column value`. Indices in the file start at one. Complex entries have four
fields, `row column real imaginary`, and require a `complex` field in the header.
The matrix must be square with positive order no greater than `INT32_MAX`.

Real samples accept `real`, `integer`, or `pattern` fields. Pattern entries
contain only `row column`; the reader assigns each a value of `1`.
Complex samples accept the `complex` field. Finite matrix values are required.

The reader supports `general`, `symmetric`, `hermitian` (complex only), and
`skew-symmetric` declarations. It mirrors off-diagonal entries; Hermitian input
uses conjugation and skew-symmetric input uses sign reversal. Hermitian diagonal
entries must have zero imaginary part; skew-symmetric diagonals must be zero.

Expansion applies to each stored entry. Repeated coordinates are retained and
contribute separately to the mathematical matrix and residual. For new symmetric
or Hermitian input, store one triangle, or use `general` for an explicitly stored
full matrix, to avoid unintended duplicate contributions. The supplied fixtures
are consumed unchanged using the same expansion rules as the solver input.

## In-Memory Sparse Arrays

Input is converted to zero-based CSC, or CSR in sample 05:

- `ptr`: signed 64-bit compressed offsets, length `n + 1`.
- `idx`: signed 32-bit row indices (CSC) or column indices (CSR).
- `values`: one real element per entry, or interleaved real/imaginary components
  for a complex element. The single-precision samples convert values to `float`.

The API descriptors use block size `1` and `PANGULU_LU`. These samples pass the
expanded full matrix. Refactorization requires identical `ptr` and `idx` arrays,
including their ordering, for both matrices; values alone may change.

## RHS Input

RHS `.vec` files have no header. For a matrix of order `n`, supply `n`
whitespace-separated real values or `n` real/imaginary pairs. Samples consume
those values in vector order; do not add a length prefix or metadata.

Single-vector samples use contiguous device buffers. Sample 08 reads its RHS
files separately, then packs columns as `data[row + column * ld]`, with `ld = n`.
The leading dimension counts elements, not bytes. Complex elements consist of
adjacent real/imaginary components.

The input files do not encode a random seed or a reference solution.
Configuration is set in the sample's C API calls, not in the matrix or RHS file.
