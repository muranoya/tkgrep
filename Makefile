.PHONY: all clean

TARGET=tkgrep
CXX=clang++
CXXFLAGS += -O2 -Wall -std=c++17 -g `llvm-config --cxxflags` -fexceptions
LDFLAGS += `llvm-config --ldflags` -lclang

all: $(TARGET)

$(TARGET): src/main.o src/Util.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

src/main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

src/Util.o: src/Util.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

clean:
	rm -fr src/*.o tkgrep
