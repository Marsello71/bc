CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3

BUILD_DIR := build
TARGET := benchmark

SRCS := $(wildcard src/*.cpp) \
        $(wildcard src/wrappers/*.cpp) \
        $(wildcard external/chaskey/*.cpp) \
        $(wildcard external/siphash/*.cpp) \
        $(wildcard external/spookyhash/*.cpp) \
        $(wildcard external/ascon/*.cpp) \
        $(wildcard external/jhash/*.cpp) \
        $(wildcard external/toeplitz/*.cpp) \
        $(wildcard external/nsgahash/*.cpp) \
        $(wildcard external/crc32c/*.cpp)       

OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) 

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
