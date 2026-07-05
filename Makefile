CXX := g++

CXXFLAGS := -g -Wall -MMD -MP
LDFLAGS :=
LDLIBS := -lsamplerate -lsndfile -lm -lfftw3f -lsqlite3 -lpthread

TARGET := exe/main

SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst src/%.cpp,build/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)

build:
	mkdir -p $@

exe:
	mkdir -p $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) | exe
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -rf build exe

-include $(DEPS)