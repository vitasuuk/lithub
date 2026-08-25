CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

TARGET = build/lithub

SOURCES = src/main.cc \
          src/LitCommands.cc \
          src/LitRepository.cc \
          src/LitConfig.cc \
          src/LitBlob.cc \
          src/LitObjectStore.cc

OBJECTS = $(SOURCES:src/%.cc=build/%.o)

$(TARGET): $(OBJECTS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) -lcrypto -lz

build/%.o: src/%.cc
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build