CC = /opt/mpich2/bin/mpicc
NVCC = nvcc
CFLAGS = -Wall -Wextra -O3 -pthread -fopenmp -I./include -DCUDA_ENABLED
CUDA_ARCH = -arch=sm_61
LDFLAGS = -L/opt/mpich2/lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu/ -lcudart -lcuda -lm -lstdc++
MPI_EXEC = /opt/mpich2/bin/mpiexec
NP = 3
# WSL fix: Hydra process manager must use loopback (eth0 binding hangs in WSL)
HYDRA_IFACE = lo

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TARGET = $(BIN_DIR)/load_balancer

C_FILES = main.c master.c worker.c listener.c compute.c config.c dashboard.c cuda_monitor.c
CU_FILES = compute.cu

C_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(C_FILES))
CU_OBJS = $(patsubst %.cu, $(OBJ_DIR)/%.cu.o, $(CU_FILES))
ALL_OBJS = $(C_OBJS) $(CU_OBJS)

.PHONY: all clean run

all: $(BIN_DIR) $(OBJ_DIR) $(TARGET)

run: all
	HYDRA_IFACE=$(HYDRA_IFACE) $(MPI_EXEC) -n $(NP) $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(TARGET): $(ALL_OBJS)
	$(CC) $(CFLAGS) -o $@ $(ALL_OBJS) $(LDFLAGS)
	@echo "Successfully created: $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.cu.o: $(SRC_DIR)/%.cu
	$(NVCC) $(CUDA_ARCH) -Xcompiler "-fPIC -fopenmp" -I/opt/mpich2/include -I./include -c $< -o $@
	@echo "Compiled: $@"

clean:
	sudo chown -R $(shell whoami) obj bin 2>/dev/null || true
	rm -rf $(OBJ_DIR) $(BIN_DIR)
