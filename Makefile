CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

SRC_DIR := src
OBJ_DIR := build
TARGET := hive_sim

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) hive_test

.PHONY: all clean

# Unit test target (no external libraries). Compiles all sources and the
# tests with UNIT_TEST defined. Enables ASAN/UBSAN for better diagnostics.
test: $(SRC_DIR)/*.cpp tests/unit_tests.cpp
	SRCS="$(filter-out $(SRC_DIR)/main.cpp,$(wildcard $(SRC_DIR)/*.cpp))" && \
	$(CXX) $(CXXFLAGS) -DUNIT_TEST -g -O1 -fsanitize=address,undefined -I$(SRC_DIR) $$SRCS tests/unit_tests.cpp -o hive_test

run-test: test
	./hive_test

.PHONY: test run-test