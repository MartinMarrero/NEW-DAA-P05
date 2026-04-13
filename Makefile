CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc -Isrc/lib -MMD -MP
TARGET := ms-cflp-ci

SRC_DIR := src
# SRC_DIR := $(SRC_DIR)/src
BUILD_DIR := build

SRCS := $(SRC_DIR)/main.cc \
	$(SRC_DIR)/instance.cc \
	$(SRC_DIR)/solution.cc \
	$(SRC_DIR)/greedy.cc \
	$(SRC_DIR)/grasp.cc \
	$(SRC_DIR)/local_search.cc \
	$(SRC_DIR)/client_swap_search.cc \
	$(SRC_DIR)/incompatibility_search.cc \
	$(SRC_DIR)/swap_search.cc \
	$(SRC_DIR)/relocation_search.cc

OBJS := $(SRCS:%.cc=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)