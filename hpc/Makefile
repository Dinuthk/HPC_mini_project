# Define the compiler and the flags
CC = mpicc
CFLAGS = -Wall -Wextra -O3 -pthread -I./include -I/opt/homebrew/opt/libomp/include -Xclang -fopenmp
LDFLAGS = -L/opt/homebrew/opt/libomp/lib -lomp

# Define directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Define the final executable name
TARGET = $(BIN_DIR)/load_balancer

# Find all .c files in the src directory
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Define the corresponding .o (object) files
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Default target to build everything
all: directories $(TARGET)

# Create necessary directories if they don't exist
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# Link all the object files to create the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile each .c file into a .o file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean command to remove compiled files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)