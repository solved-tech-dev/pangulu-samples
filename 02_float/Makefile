# Build this sample against an existing PanguLU SDK; do not rebuild the SDK.
# SDK_ROOT defaults to the containing SDK when installed as SDK/pangulu_samples/.
CC ?= gcc
CFLAGS ?= -O3 -std=c11 -Wall -Wextra -Wno-unused-function
SDK_ROOT ?= $(if $(PANGULU_SDK),$(PANGULU_SDK),$(abspath $(CURDIR)/../..))
CUDA_ROOT ?= /usr/local/cuda
CUDA_LIB_DIR ?= $(if $(wildcard $(CUDA_ROOT)/lib64),$(CUDA_ROOT)/lib64,$(CUDA_ROOT)/lib)
GPU_INDEX ?= 0
RUN_ARGS ?= data/matrix_real.mtx data/rhs_real_1.vec
TARGET := sample

CPPFLAGS += -I"$(SDK_ROOT)/include" -I"$(CUDA_ROOT)/include"
LDFLAGS += -L"$(SDK_ROOT)/lib" -L"$(CUDA_LIB_DIR)" \
    -Wl,-rpath,'$$ORIGIN/../../lib' -Wl,-rpath,"$(SDK_ROOT)/lib" \
    -Wl,-rpath,"$(CUDA_LIB_DIR)" \
    -Wl,--no-as-needed -Wl,--allow-shlib-undefined
LDLIBS += -lpangulu -lpangulu_platform -lcudart -ldl -lm

.PHONY: all check run clean
all: $(TARGET)

check:
	@test -f "$(SDK_ROOT)/include/pangulu.h" || { \
	    printf '%s\n' 'Set SDK_ROOT to the extracted PanguLU SDK directory.' >&2; exit 1; }
	@test -f "$(SDK_ROOT)/lib/libpangulu.so" || { \
	    printf '%s\n' 'Missing SDK library: lib/libpangulu.so' >&2; exit 1; }
	@test -f "$(SDK_ROOT)/lib/libpangulu_platform.so" || { \
	    printf '%s\n' 'Missing SDK library: lib/libpangulu_platform.so' >&2; exit 1; }
	@test -f "$(CUDA_ROOT)/include/cuda_runtime.h" || { \
	    printf '%s\n' 'Set CUDA_ROOT to a compatible CUDA development toolkit.' >&2; exit 1; }
	@test -f "$(CUDA_LIB_DIR)/libcudart.so" || { \
	    printf '%s\n' 'Set CUDA_LIB_DIR to the directory containing libcudart.so.' >&2; exit 1; }

$(TARGET): sample.c sample_common.h | check
	$(CC) $(CPPFLAGS) $(CFLAGS) sample.c -o $@ $(LDFLAGS) $(LDLIBS)

run: all
	LD_LIBRARY_PATH="$(SDK_ROOT)/lib:$(CUDA_LIB_DIR)$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}" \
	    ./$(TARGET) -g "$(GPU_INDEX)" $(RUN_ARGS)

clean:
	$(RM) $(TARGET)
