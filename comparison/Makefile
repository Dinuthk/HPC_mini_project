# Comparison Makefile - Compile all versions for benchmarking

CC = gcc
CFLAGS_SEQ = -Wall -Wextra -O3
CFLAGS_MT = -Wall -Wextra -O3 -pthread
MPI_CC = mpicc
MPI_CFLAGS = -Wall -Wextra -O3 -pthread

BIN_DIR = bin

all: seq mt

# Sequential version
seq: $(BIN_DIR)/sequential
	@echo "✓ Sequential version compiled: $(BIN_DIR)/sequential"

$(BIN_DIR)/sequential: sequential.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS_SEQ) sequential.c -o $(BIN_DIR)/sequential

# Multi-threaded version
mt: $(BIN_DIR)/multithreaded
	@echo "✓ Multi-threaded version compiled: $(BIN_DIR)/multithreaded"

$(BIN_DIR)/multithreaded: multithreaded.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS_MT) multithreaded.c -o $(BIN_DIR)/multithreaded

# Clean
clean:
	@rm -rf $(BIN_DIR)
	@echo "✓ Cleaned up binaries"

# Help
help:
	@echo "Comparison Makefile - Compile versions for benchmarking"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Compile both sequential and multi-threaded versions"
	@echo "  make seq      - Compile sequential version only"
	@echo "  make mt       - Compile multi-threaded version only"
	@echo "  make clean    - Remove compiled binaries"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make && ./bin/sequential && ./bin/multithreaded"

.PHONY: all seq mt clean help
