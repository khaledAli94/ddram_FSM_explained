# Compiler and Flags
CC       := gcc
CFLAGS   := -Wall -Wextra -Werror -std=c99 -Isrc
TARGET   := ddr3_sim

# Directories
SRC_DIR  := src
BUILD_DIR:= build

# Source and Object Files
SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Default Rule
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "Build successful: ./$(TARGET)"

# Compile C source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild