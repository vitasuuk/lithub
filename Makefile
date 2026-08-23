CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

TARGET = build/lithub

SOURCES = src/main.cc src/LitCommands.cc src/LitRepository.cc src/LitConfig.cc
OBJECTS = $(SOURCES:.cc=.o)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)