# No Load Balancer - Pure Sequential (Baseline Comparison)
# NO MPI, NO OpenMP — plain gcc
CC = gcc
CFLAGS = -Wall -Wextra -O3

BIN_DIR = bin
TARGET = $(BIN_DIR)/no_balancer

all: directories $(TARGET)

directories:
	@mkdir -p $(BIN_DIR)

$(TARGET): no_balancer.c
	$(CC) $(CFLAGS) no_balancer.c -o $(TARGET) -lm -pthread

clean:
	rm -rf $(BIN_DIR)

.PHONY: all directories clean
