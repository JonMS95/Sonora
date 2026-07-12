# -----------------------------------------------------------------------------
# Usage examples
# -----------------------------------------------------------------------------
# make lib           	   		# Build only libsonora.so (debug)
# make cli               		# Build sonora-cli (and libsonora.so if needed)
# make                   		# Build both
# make BUILD=release			# Build both in release mode
# make lib BUILD=release
# make cli BUILD=release
# make tests              # Build only the unit tests (and lib if needed)
# make BUILD=release
# make clean

CXX := g++

# -----------------------------------------------------------------------------
# Build mode
# -----------------------------------------------------------------------------

BUILD ?= debug

# -----------------------------------------------------------------------------
# Project layout
# -----------------------------------------------------------------------------

LIB_NAME := sonora

APP_NAME := sonora-cli
TEST_NAME := unit_tests

SRC_DIR  := src
APP_DIR  := apps/cli/src
TEST_DIR := tests/unit/src

BUILD_DIR := build/$(BUILD)

OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/deps
LIB_DIR := $(BUILD_DIR)/lib

BIN_DIR := exe/$(BUILD)

LIB_TARGET  := $(LIB_DIR)/lib$(LIB_NAME).so
APP_TARGET  := $(BIN_DIR)/$(APP_NAME)
TEST_TARGET := $(BIN_DIR)/$(TEST_NAME)

TEST_FILES_DIR := $(CURDIR)/tests/unit/data

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

LIB_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
APP_SRCS := $(wildcard $(APP_DIR)/*.cpp)
TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)

LIB_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/src/%.o,$(LIB_SRCS))
APP_OBJS := $(patsubst $(APP_DIR)/%.cpp,$(OBJ_DIR)/apps/cli/%.o,$(APP_SRCS))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(OBJ_DIR)/tests/unit/%.o,$(TEST_SRCS))

LIB_DEPS := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(LIB_OBJS))
APP_DEPS := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(APP_OBJS))
TEST_DEPS := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(TEST_OBJS))

# -----------------------------------------------------------------------------
# Compiler / linker flags
# -----------------------------------------------------------------------------

CPPFLAGS :=
CXXFLAGS := -std=c++20 -Wall -Wextra -fPIC
DEPFLAGS := -MMD -MP

PUBLIC_INC 	:= -Iinclude
PRIVATE_INC := -Isrc

LDFLAGS :=

LDLIBS := \
	-lsamplerate \
	-lsndfile \
	-lm \
	-lfftw3f \
	-lsqlite3 \
	-latomic \
	-lpthread

TEST_CPPFLAGS := -DTEST_DATA_DIR=\"$(TEST_FILES_DIR)\"

# -----------------------------------------------------------------------------
# Build configuration
# -----------------------------------------------------------------------------

ifeq ($(BUILD),debug)

	CXXFLAGS += -g -O0 -fno-omit-frame-pointer

	SANITIZE := -fsanitize=address -fsanitize=undefined

	CXXFLAGS += $(SANITIZE)
	LDFLAGS += $(SANITIZE)

else ifeq ($(BUILD),release)

	CXXFLAGS += -O2 -DNDEBUG

else

	$(error Unknown BUILD=$(BUILD). Use debug or release)

endif

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

.PHONY: all lib cli tests clean debug release

all: lib cli tests

lib: $(LIB_TARGET)

cli: $(APP_TARGET)

tests: $(TEST_TARGET)

debug:
	$(MAKE) BUILD=debug

release:
	$(MAKE) BUILD=release

clean:
	rm -rf build exe

# -----------------------------------------------------------------------------
# Library sources
# -----------------------------------------------------------------------------

$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PUBLIC_INC) $(DEPFLAGS) \
		-MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) \
		-c $< -o $@

# -----------------------------------------------------------------------------
# CLI sources
# -----------------------------------------------------------------------------

$(OBJ_DIR)/apps/cli/%.o: $(APP_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(PUBLIC_INC) $(DEPFLAGS) \
		-MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) \
		-c $< -o $@

# -----------------------------------------------------------------------------
# Test sources
# -----------------------------------------------------------------------------

$(OBJ_DIR)/tests/unit/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TEST_CPPFLAGS) $(PUBLIC_INC) $(PRIVATE_INC) $(DEPFLAGS) \
		-MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) \
		-c $< -o $@

# -----------------------------------------------------------------------------
# Shared library
# -----------------------------------------------------------------------------

$(LIB_TARGET): $(LIB_OBJS)
	@mkdir -p $(LIB_DIR)
	$(CXX) -shared $(LDFLAGS) $^ -o $@ $(LDLIBS)

# -----------------------------------------------------------------------------
# CLI executable
# -----------------------------------------------------------------------------

$(APP_TARGET): $(APP_OBJS) $(LIB_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDFLAGS) $^ \
		-L$(LIB_DIR) \
		-l$(LIB_NAME) \
		-Wl,-rpath,'$$ORIGIN/../../build/$(BUILD)/lib' \
		-o $@

# -----------------------------------------------------------------------------
# Unit test executable
# -----------------------------------------------------------------------------

$(TEST_TARGET): $(TEST_OBJS) $(LIB_TARGET)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDFLAGS) $^ \
		-L$(LIB_DIR) \
		-l$(LIB_NAME) \
		$(LDLIBS) \
		-Wl,-rpath,'$$ORIGIN/../../build/$(BUILD)/lib' \
		-o $@

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

-include $(LIB_DEPS)
-include $(APP_DEPS)
-include $(TEST_DEPS)