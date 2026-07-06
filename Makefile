# -----------------------------------------------------------------------------
# Usage examples
# -----------------------------------------------------------------------------
# make lib           	   		# Build only libsonora.so (debug)
# make cli               		# Build sonora-cli (and libsonora.so if needed)
# make                   		# Build both
# make BUILD=release			# Build both in release mode
# make lib BUILD=release
# make cli BUILD=release
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

SRC_DIR := src
APP_DIR := apps/cli/src

BUILD_DIR := build/$(BUILD)

OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/deps
LIB_DIR := $(BUILD_DIR)/lib

BIN_DIR := exe/$(BUILD)

LIB_TARGET := $(LIB_DIR)/lib$(LIB_NAME).so
APP_TARGET := $(BIN_DIR)/$(APP_NAME)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

LIB_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
APP_SRCS := $(wildcard $(APP_DIR)/*.cpp)

LIB_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/src/%.o,$(LIB_SRCS))
APP_OBJS := $(patsubst $(APP_DIR)/%.cpp,$(OBJ_DIR)/apps/cli/%.o,$(APP_SRCS))

LIB_DEPS := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(LIB_OBJS))
APP_DEPS := $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$(APP_OBJS))

# -----------------------------------------------------------------------------
# Compiler / linker flags
# -----------------------------------------------------------------------------

CPPFLAGS :=
CXXFLAGS := -std=c++20 -Wall -Wextra -Iinclude -fPIC
DEPFLAGS := -MMD -MP

LDFLAGS :=
LDLIBS := -lsamplerate -lsndfile -lm -lfftw3f -lsqlite3 -lpthread

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

.PHONY: all lib cli clean debug release

all: lib cli

lib: $(LIB_TARGET)

cli: $(APP_TARGET)

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
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) \
		-MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) \
		-c $< -o $@

# -----------------------------------------------------------------------------
# CLI sources
# -----------------------------------------------------------------------------

$(OBJ_DIR)/apps/cli/%.o: $(APP_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) \
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
	$(CXX) $(LDFLAGS) $< \
		-L$(LIB_DIR) \
		-l$(LIB_NAME) \
		-Wl,-rpath,'$$ORIGIN/../../build/$(BUILD)/lib' \
		-o $@

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

-include $(LIB_DEPS)
-include $(APP_DEPS)
