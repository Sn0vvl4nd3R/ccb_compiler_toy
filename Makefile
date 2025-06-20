CC = gcc

CFLAGS = -g -Wall -Wextra

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

EXECUTABLE = $(BIN_DIR)/compiler

SOURCES = $(wildcard $(SRC_DIR)/**/*.c) $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Cleaning up..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)

test: all
	@echo "Running test..."
	./$(EXECUTABLE) examples/program.ccb

.PHONY: all clean test
