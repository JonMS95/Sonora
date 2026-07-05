CXX := g++

# --- Build mode selection ---
BUILD ?= debug

# --- Project layout ---
TARGET_NAME := main

SRCDIR := src
BUILDDIR := build/$(BUILD)
BINDIR := exe/$(BUILD)

TARGET := $(BINDIR)/$(TARGET_NAME)

# --- Sources / objects ---
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# --- Common flags ---
CPPFLAGS :=
CXXFLAGS := -Wall -MMD -MP
LDFLAGS :=
LDLIBS := -lsamplerate -lsndfile -lm -lfftw3f -lsqlite3 -lpthread

# --- Build modes ---
ifeq ($(BUILD),debug)
	CXXFLAGS += -g -O0 -fno-omit-frame-pointer
	SANITIZE := -fsanitize=address -fsanitize=undefined
	CXXFLAGS += $(SANITIZE)
	LDFLAGS  += $(SANITIZE)

else ifeq ($(BUILD),release)
	CXXFLAGS += -O2 -DNDEBUG

else
	$(error Unknown BUILD=$(BUILD). Use debug or release)
endif

# --- Phony targets ---
.PHONY: all clean run debug release

all: $(TARGET)

debug:
	$(MAKE) BUILD=debug

release:
	$(MAKE) BUILD=release

clean:
	rm -rf build exe

# --- Directory creation ---
$(BUILDDIR):
	mkdir -p $@

$(BINDIR):
	mkdir -p $@

# --- Compile ---
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# --- Link ---
$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# --- Dependencies ---
-include $(DEPS)
