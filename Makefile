CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -Icore/include
SOURCES = core/src/*.cpp test_example.cpp
TARGET = cachesim_test

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(SOURCES)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

# For Windows with MinGW
windows:
	g++ -std=c++17 -Wall -Wextra -O2 -Icore/include -o cachesim_test.exe core/src/*.cpp test_example.cpp

# For macOS/Linux
unix:
	g++ -std=c++17 -Wall -Wextra -O2 -Icore/include -o cachesim_test core/src/*.cpp test_example.cpp
