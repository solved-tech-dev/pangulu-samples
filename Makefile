# Build all samples through their local inputs and shared rules.
SAMPLE_DIRS := 01_double 02_float 03_double_complex 04_float_complex 05_csr 06_cpu_matrix_input 07_l0_reordering 08_multi_rhs 09_refactor \
    10_reordering_nd 11_reordering_amd 12_reordering_rcm 13_reordering_btf \
    14_reordering_btf_colamd 15_reordering_dm 16_reordering_given_perm 17_reordering_none
.PHONY: all clean $(SAMPLE_DIRS)
all: $(SAMPLE_DIRS)
$(SAMPLE_DIRS):
	$(MAKE) -C $@ all
clean:
	@set -e; for dir in $(SAMPLE_DIRS); do $(MAKE) -C "$$dir" clean; done
