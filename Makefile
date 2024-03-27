CC = g++
CFLAGS = -std=c++11 -Wall -fPIC
GTEST_LDFLAGS = -lgtest -lgtest_main -pthread
THREADPOOL_LDFLAGS = -pthread
BENCHMARK_LDFLAGS = -lbenchmark -pthread

SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin

# Source files for the ThreadPool executable
THREADPOOL_SRC = $(SRC_DIR)/threadpool.cpp
THREADPOOL_OBJ = $(OBJ_DIR)/threadpool.o
THREADPOOL_TARGET = $(BIN_DIR)/ThreadPool

# Source files for the gtest executable
GTEST_SRC = $(SRC_DIR)/gtest.cpp
GTEST_OBJ = $(OBJ_DIR)/gtest.o
GTEST_TARGET = $(BIN_DIR)/gtest

all: $(THREADPOOL_TARGET) $(GTEST_TARGET)

$(THREADPOOL_TARGET): $(THREADPOOL_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(THREADPOOL_LDFLAGS) $(BENCHMARK_LDFLAGS)

$(GTEST_TARGET): $(GTEST_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(GTEST_LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

.PHONY: all clean