# Build the independent samples using their local Makefiles.
SAMPLE_DIRS := 01_double 02_float 03_dcomplex 04_fcomplex 05_csr 06_cpu 07_l0 08_multi_rhs 09_refactor
.PHONY: all clean $(SAMPLE_DIRS)
all: $(SAMPLE_DIRS)
$(SAMPLE_DIRS):
	$(MAKE) -C $@ all
clean:
	@set -e; for dir in $(SAMPLE_DIRS); do $(MAKE) -C "$$dir" clean; done
