# Shared build rules for samples; the selected package supplies the product.
.DEFAULT_GOAL := all
SAMPLES_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PACKAGE_HEADERS := $(wildcard $(SAMPLES_ROOT)/libraries/*/include/pangulu.h)
PACKAGE_DIRS := $(patsubst %/include/pangulu.h,%,$(PACKAGE_HEADERS))
LIBRARY_ROOT ?= $(if $(filter 1,$(words $(PACKAGE_DIRS))),$(firstword $(PACKAGE_DIRS)))
LIBRARY_DIR := $(if $(strip $(LIBRARY_ROOT)),$(abspath $(LIBRARY_ROOT)))
TOOLKIT_ROOT ?= $(if $(CUDA_ROOT),$(CUDA_ROOT),$(if $(CUDA_HOME),$(CUDA_HOME),/usr/local/cuda))
TOOLKIT_LIB_DIR ?= $(if $(CUDA_LIB_DIR),$(CUDA_LIB_DIR),$(if $(wildcard $(TOOLKIT_ROOT)/lib64),$(TOOLKIT_ROOT)/lib64,$(TOOLKIT_ROOT)/lib))
PANGULU_LICENSE_PATHS ?= $(LIBRARY_DIR)/license.key
GPU_INDEX ?= 0
RUN_ARGS ?= $(DEFAULT_INPUTS)
TARGET := sample
CC ?= gcc
CFLAGS ?= -O3 -std=c11 -Wall -Wextra -Wno-unused-function
CPPFLAGS += -I"$(SAMPLES_ROOT)/common" -I"$(LIBRARY_DIR)/include" -I"$(TOOLKIT_ROOT)/include"
LDFLAGS += -L"$(LIBRARY_DIR)/lib" -L"$(TOOLKIT_LIB_DIR)" \
    -Wl,-rpath,"$(LIBRARY_DIR)/lib" -Wl,-rpath,"$(TOOLKIT_LIB_DIR)" \
    -Wl,--no-as-needed -Wl,--allow-shlib-undefined
LDLIBS += -lpangulu -lpangulu_platform -lcudart -ldl -lm

.PHONY: all check run clean
all: $(TARGET)

check:
	@test -n "$(LIBRARY_DIR)" || { \
	    printf '%s\n' 'Place exactly one package in libraries/, or set LIBRARY_ROOT=/path/to/package.' >&2; exit 1; }
	@test -f "$(LIBRARY_DIR)/include/pangulu.h" || { \
	    printf '%s\n' 'Missing include/pangulu.h; set LIBRARY_ROOT to the extracted package directory.' >&2; exit 1; }
	@test -f "$(LIBRARY_DIR)/lib/libpangulu.so" || { \
	    printf '%s\n' 'Missing package library: lib/libpangulu.so' >&2; exit 1; }
	@test -f "$(LIBRARY_DIR)/lib/libpangulu_platform.so" || { \
	    printf '%s\n' 'Missing package library: lib/libpangulu_platform.so' >&2; exit 1; }
	@test -f "$(TOOLKIT_ROOT)/include/cuda_runtime.h" || { \
	    printf '%s\n' 'Set TOOLKIT_ROOT to the development toolkit required by the selected package.' >&2; exit 1; }
	@test -f "$(TOOLKIT_LIB_DIR)/libcudart.so" || { \
	    printf '%s\n' 'Set TOOLKIT_LIB_DIR to the runtime library directory required by the selected package.' >&2; exit 1; }

$(TARGET): sample.c $(SAMPLES_ROOT)/common/sample_common.h Makefile $(SAMPLES_ROOT)/common.mk | check
	$(CC) $(CPPFLAGS) $(CFLAGS) sample.c -o $@ $(LDFLAGS) $(LDLIBS)

run: all
	PANGULU_LICENSE_PATHS="$(PANGULU_LICENSE_PATHS)" \
	LD_LIBRARY_PATH="$(LIBRARY_DIR)/lib:$(TOOLKIT_LIB_DIR)$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}" \
	    ./$(TARGET) -g "$(GPU_INDEX)" $(RUN_ARGS)

clean:
	$(RM) $(TARGET)
