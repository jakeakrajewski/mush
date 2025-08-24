CC = clang
CFLAGS = -Wall -Wextra -g -O0 -Iinclude
OBJ_DIR = build
SRC_DIR = src
PROMPTLY_DIR = src/promptly

# Include both src and promptly source files
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(PROMPTLY_DIR)/*.c)

# Generate object files for both directories
SRC_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c))
PROMPTLY_OBJS = $(patsubst $(PROMPTLY_DIR)/%.c,$(OBJ_DIR)/promptly_%.o,$(wildcard $(PROMPTLY_DIR)/*.c))
OBJS = $(SRC_OBJS) $(PROMPTLY_OBJS)

TARGET = $(OBJ_DIR)/mu

# Rule for src directory
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for promptly directory
$(OBJ_DIR)/promptly_%.o: $(PROMPTLY_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

all: $(TARGET)

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all clean
