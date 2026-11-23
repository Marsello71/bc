CXX := g++
# -MMD -MP: remember which headers every .cpp includes, so changing a header rebuilds it
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -MMD -MP -Isrc

BUILD_DIR := build
TARGET := rss_sim

SRCS := $(wildcard src/*.cpp) \
        $(wildcard src/wrappers/*.cpp) \
        $(wildcard external/chaskey/*.cpp) \
        $(wildcard external/siphash/*.cpp) \
        $(wildcard external/jhash/*.cpp) \
        $(wildcard external/toeplitz/*.cpp) \
        $(wildcard external/nh/*.cpp)

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
	rm -rf $(BUILD_DIR) $(TARGET) benchmark

-include $(OBJS:.o=.d)
