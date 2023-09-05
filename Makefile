# Makefile for ThreadPool_TwoMode

# Compiler settings
CC = g++
CFLAGS = -std=c++14 -Wall -Wextra -pthread

# Source and object file settings
SRC_DIR = .
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_DIR = obj
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Executable file setting
EXECUTABLE = ThreadPool_TwoMode

# Default target (compiling the executable)
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean generated files
clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE)

.PHONY: all clean
