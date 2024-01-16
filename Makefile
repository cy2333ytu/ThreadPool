CC = g++
CFLAGS = -std=c++11 -Wall -fPIC
LDFLAGS = -pthread -lbenchmark

SRC_DIR = .
OBJ_DIR = obj
LIB_DIR = lib
BIN_DIR = bin

SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
TARGET = $(LIB_DIR)/libThreadPool.so
EXECUTABLE = $(BIN_DIR)/ThreadPool

all: $(TARGET) $(EXECUTABLE)

$(TARGET): $(OBJ_FILES)
	@mkdir -p $(LIB_DIR)
	$(CC) $(CFLAGS) -shared $^ -o $@ $(LDFLAGS)

$(EXECUTABLE): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

.PHONY: all clean
