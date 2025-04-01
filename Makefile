# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -Wno-unused-parameter -Wno-unused-variable -O2
LDFLAGS = -lrt -lncurses

# Directory structure
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# File discovery
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))
TARGET := $(BIN_DIR)/app

# Ensure directories exist
$(shell mkdir -p $(BUILD_DIR) $(BIN_DIR))

# Default target
all: $(TARGET)

# Link executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Run the application
run: $(TARGET)
	@echo "Running application..."
	./$(TARGET)

# Print debug info
print:
	@echo "SRC_DIR: $(SRC_DIR)"
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJECTS: $(OBJECTS)"
	@echo "Working directory: $(shell pwd)"

.PHONY: all clean run print