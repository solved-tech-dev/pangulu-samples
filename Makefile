# Build all samples through their local inputs and shared rules.
SAMPLE_DIRS := 01_double 02_float 03_double_complex 04_float_complex 05_csr 06_cpu_matrix_input 07_l0_reordering 08_multi_rhs 09_refactor
.PHONY: all clean $(SAMPLE_DIRS)
all: $(SAMPLE_DIRS)
$(SAMPLE_DIRS):
	$(MAKE) -C $@ all
clean:
	@set -e; for dir in $(SAMPLE_DIRS); do $(MAKE) -C "$$dir" clean; done
