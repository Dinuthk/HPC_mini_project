#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <omp.h>

#define TAG_WORK 1
#define TAG_KILL_SIGNAL 2
#define MAX_QUEUE_SIZE 200000
#define BATCH_SIZE 1024 // How many trades to send to the GPU at once

// 1. The Trade Struct
typedef struct {
    int stock_id;
    double price;
    double volume;
    long timestamp;
    double complexity_weight;
} Trade;

// 2. Thread-Safe Queue (Same as Phase 2)
typedef struct {
    Trade data[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
} TradeQueue;

void init_queue(TradeQueue* q) { q->front = 0; q->rear = -1; q->count = 0; }

void push_queue(TradeQueue* q, Trade t) {
    if (q->count < MAX_QUEUE_SIZE) {
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        q->data[q->rear] = t;
        q->count++;
    }
}

Trade pop_queue(TradeQueue* q) {
    Trade t = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->count--;
    return t;
}

// Shared Globals
TradeQueue task_queue;
pthread_mutex_t queue_mutex;
int simulation_running = 1;

// ---------------------------------------------------------
// CUDA KERNEL: Mock Monte Carlo Risk Simulation
// ---------------------------------------------------------
// This runs on the NVIDIA GPU. 
__global__ void monte_carlo_risk_kernel(Trade* d_trades, double* d_risk_results, int num_trades) {
    // Find which GPU thread this is
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < num_trades) {
        double price = d_trades[idx].price;
        double vol = d_trades[idx].volume;
        double risk = 0.0;

        // Mock heavy calculation (Simulating 1000 Monte Carlo paths)
        for (int i = 0; i < 1000; i++) {
            // Some arbitrary math to make the GPU work hard
            risk += (price * 0.01) + (vol * 0.0001) - (i * 0.001); 
        }
        
        d_risk_results[idx] = risk;
    }
}

// ---------------------------------------------------------
// BACKGROUND THREAD: The Asynchronous Listener
// ---------------------------------------------------------
void* network_listener(void* arg) {
    // ... [Exactly the same as Phase 2] ...
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    while (simulation_running) {
        int flag = 0;
        MPI_Status status;
        MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            if (status.MPI_TAG == TAG_WORK) {
                int incoming_bytes;
                MPI_Get_count(&status, MPI_BYTE, &incoming_bytes);
                int num_trades = incoming_bytes / sizeof(Trade);

                Trade* incoming_batch = (Trade*)malloc(incoming_bytes);
                MPI_Recv(incoming_batch, incoming_bytes, MPI_BYTE, 0, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                pthread_mutex_lock(&queue_mutex);
                for (int i = 0; i < num_trades; i++) push_queue(&task_queue, incoming_batch[i]);
                printf("\n[Worker %d Listener] INTERRUPT: Received %d new trades! Queue: %d\n", world_rank, num_trades, task_queue.count);
                pthread_mutex_unlock(&queue_mutex);

                free(incoming_batch);
            } 
            else if (status.MPI_TAG == TAG_KILL_SIGNAL) {
                int kill_msg;
                MPI_Recv(&kill_msg, 1, MPI_INT, 0, TAG_KILL_SIGNAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                simulation_running = 0; 
            }
        }
        usleep(10000); 
    }
    return NULL;
}

// ---------------------------------------------------------
// MAIN EXECUTABLE
// ---------------------------------------------------------
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_rank == 0) {
        // ... [Master Node logic exactly the same as Phase 2] ...
        printf("[Master] Booting market simulation...\n");
        Trade* batch = (Trade*)malloc(50000 * sizeof(Trade));
        for (int i = 0; i < 50000; i++) { batch[i].stock_id = i; batch[i].price = 100.0; batch[i].volume = 10.0; }

        printf("[Master] Sending morning trades (50,000 each)...\n");
        MPI_Send(batch, 50000 * sizeof(Trade), MPI_BYTE, 1, TAG_WORK, MPI_COMM_WORLD);
        MPI_Send(batch, 50000 * sizeof(Trade), MPI_BYTE, 2, TAG_WORK, MPI_COMM_WORLD);

        sleep(2); 
        int kill = 1;
        MPI_Send(&kill, 1, MPI_INT, 1, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
        MPI_Send(&kill, 1, MPI_INT, 2, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
        free(batch);
    } 
    else {
        init_queue(&task_queue);
        pthread_mutex_init(&queue_mutex, NULL);
        
        pthread_t listener;
        pthread_create(&listener, NULL, network_listener, NULL);

        // CPU Memory buffers for the batch
        Trade host_trades[BATCH_SIZE];
        double host_results[BATCH_SIZE];

        // GPU Memory pointers
        Trade* device_trades;
        double* device_results;
        cudaMalloc((void**)&device_trades, BATCH_SIZE * sizeof(Trade));
        cudaMalloc((void**)&device_results, BATCH_SIZE * sizeof(double));

        int processed_count = 0;

        while (simulation_running || task_queue.count > 0) {
            int items_to_process = 0;

            // 1. SAFELY POP A BATCH FROM THE QUEUE
            pthread_mutex_lock(&queue_mutex);
            while (task_queue.count > 0 && items_to_process < BATCH_SIZE) {
                host_trades[items_to_process] = pop_queue(&task_queue);
                items_to_process++;
            }
            pthread_mutex_unlock(&queue_mutex);

            if (items_to_process > 0) {
                // 2. OPENMP: CPU Data Preparation
                // Spin up all laptop CPU cores to prep the data before GPU takes over
                #pragma omp parallel for
                for (int i = 0; i < items_to_process; i++) {
                    host_results[i] = 0.0; // Initialize results array
                    // Simulate some CPU prep work
                    host_trades[i].price += 1.5; 
                }

                // 3. CUDA MEMCPY: Send data to GPU VRAM
                cudaMemcpy(device_trades, host_trades, items_to_process * sizeof(Trade), cudaMemcpyHostToDevice);

                // 4. LAUNCH GPU KERNEL
                int threadsPerBlock = 256;
                int blocksPerGrid = (items_to_process + threadsPerBlock - 1) / threadsPerBlock;
                monte_carlo_risk_kernel<<<blocksPerGrid, threadsPerBlock>>>(device_trades, device_results, items_to_process);

                // Wait for GPU to finish
                cudaDeviceSynchronize();

                // 5. CUDA MEMCPY: Bring results back to CPU RAM
                cudaMemcpy(host_results, device_results, items_to_process * sizeof(double), cudaMemcpyDeviceToHost);

                processed_count += items_to_process;
                printf("[Worker %d Main] Processed batch of %d trades on GPU. Total: %d\n", world_rank, items_to_process, processed_count);
            } else {
                usleep(50000); 
            }
        }

        // Clean up GPU and threads
        cudaFree(device_trades);
        cudaFree(device_results);
        pthread_join(listener, NULL);
        pthread_mutex_destroy(&queue_mutex);
        printf("[Worker %d] Shutting down successfully.\n", world_rank);
    }

    MPI_Finalize();
    return 0;
}