#include "sample_ordering.h"

int main(int argc, char **argv)
{
    /* Each zero-based array maps an index in the new order to the original index. */
    int row_perm[] = {2, 0, 3, 1};
    int column_perm[] = {1, 3, 0, 2};
    const sample_permutation_t permutation = {
        .order = 4, .rows = row_perm, .columns = column_perm
    };
    return run_ordering_sample(argc, argv, PANGULU_REORDERING_ALGO_GIVEN_PERM,
                               "GIVEN_PERM", &permutation);
}
