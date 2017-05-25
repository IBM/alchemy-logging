# Simple makefile that wraps common bazel build commands

## Helper Vars #################################################################

BUILD_CMD = bazel build $(STD_ARGS)
TEST_CMD = bazel test $(STD_ARGS)
BASE_DIR=$(shell pwd)
TEST_ARGS = --test_env="TEST_DATA_DIR=$(BASE_DIR)"
ALL_TARGET = //...:all

## Builds ######################################################################

.PHONY: all
all:
	$(BUILD_CMD) $(ALL_TARGET)

.PHONY: deps
deps:
	bazel fetch $(ALL_TARGET)

## Tests #######################################################################

test:
	$(TEST_CMD) $(TEST_ARGS) --test_output=errors $(ALL_TARGET)

test_verbose:
	$(TEST_CMD) $(TEST_ARGS) --test_output=all $(ALL_TARGET)
