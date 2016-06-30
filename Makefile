# -----------------------------------------------------------------------------
# CMake project wrapper Makefile ----------------------------------------------
# -----------------------------------------------------------------------------

SHELL := /bin/bash
RM    := rm -rf
MKDIR := mkdir -p
BUILD_DIR := build
GENERATOR ?= Xcode

all: ./$(BUILD_DIR)/Makefile
	@ $(MAKE) -C $(BUILD_DIR) -j8

./$(BUILD_DIR)/Makefile:
	@  ($(MKDIR) $(BUILD_DIR) > /dev/null)
	@  (cd $(BUILD_DIR) > /dev/null 2>&1 && cmake ..)

clean:
	@ $(MAKE) -C $(BUILD_DIR) clean

bench:
	@  (cd $(BUILD_DIR) > /dev/null && ctest -L bench --verbose)

test:
	@  (cd $(BUILD_DIR) > /dev/null && ctest -L unit --verbose)

workspace:
	@  ($(MKDIR) $(BUILD_DIR) > /dev/null)
	@ (cd $(BUILD_DIR) > /dev/null && cmake -G $(GENERATOR) ..)

distclean:
	@- $(RM) -rf ./$(BUILD_DIR)
