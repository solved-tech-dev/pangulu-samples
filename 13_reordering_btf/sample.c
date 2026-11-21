#include "sample_ordering.h"

int main(int argc, char **argv)
{
    return run_ordering_sample(argc, argv, PANGULU_REORDERING_ALGO_BTF,
                               "BTF", NULL);
}
