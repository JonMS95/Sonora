.PHONY: prepare

dir_build:
	mkdir -p build

build/%.o: dir_build src/%.cpp
	$(CXX) -c $< -o $@