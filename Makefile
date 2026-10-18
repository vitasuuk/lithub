CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic \
	-Iexternal/argparse/include \
	-Isrc

TARGET = build/lithub

SOURCES = $(wildcard src/*.cc src/*/*.cc)

OBJECTS = $(SOURCES:src/%.cc=build/%.o)

$(TARGET): $(OBJECTS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) -lcrypto -lz

build/%.o: src/%.cc
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build

.PHONY: clean