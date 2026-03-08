#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <omp.h>
#include <cuda_runtime.h>

#define TAG_WORK 1
#define TAG_KILL_SIGNAL 2
#define MAX_QUEUE_SIZE 200000
#define BATCH_SIZE 256  // Safe for MX330 2GB VRAM

// ------------------- Trade Struct -------------------
typedef struct {
    int stock_id;
    double price;
    double volume;
    long timestamp;
    double complexity_weight;
} Trade;

// ------------------- Thread-Safe Queue -------------------
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

// ------------------- Shared Globals -------------------
TradeQueue task_queue;
pthread_mutex_t queue_mutex;
int simulation_running = 1;

// ------------------- CUDA Kernel -------------------
__global__ void monte_carlo_risk_kernel(Trade* d_trades, double* d_risk_results, int num_trades) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < num_trades) {
        double price = d_trades[idx].price;
        double vol = d_trades[idx].volume;
        double risk = 0.0;

        for (int i = 0; i < 1000; i++) {
            risk += (price * 0.01) + (vol * 0.0001) - (i * 0.001);
        }
        d_risk_results[idx] = risk;
    }
}

// ------------------- Listener Thread -------------------
void* network_listener(void* arg) {
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
                printf("[Worker %d Listener] Received %d trades! Queue size: %d\n", world_rank, num_trades, task_queue.count);
                pthread_mutex_unlock(&queue_mutex);

                free(incoming_batch);
            } else if (status.MPI_TAG == TAG_KILL_SIGNAL) {
                int kill_msg;
                MPI_Recv(&kill_msg, 1, MPI_INT, 0, TAG_KILL_SIGNAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                simulation_running = 0;
            }
        }
        usleep(10000);
    }
    return NULL;
}

// ------------------- MAIN -------------------
int main(int argc, char** argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    if (provided < MPI_THREAD_FUNNELED) {
        printf("MPI does not provide required threading support.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_rank == 0) {
        printf("[Master] Booting market simulation...\n");
        Trade* batch = (Trade*)malloc(50000 * sizeof(Trade));
        for (int i = 0; i < 50000; i++) {
            batch[i].stock_id = i;
            batch[i].price = 100.0;
            batch[i].volume = 10.0;
        }

        printf("[Master] Sending morning trades...\n");
        for (int i = 1; i < world_size; i++) {
            MPI_Send(batch, 50000 * sizeof(Trade), MPI_BYTE, i, TAG_WORK, MPI_COMM_WORLD);
        }

        sleep(2);

        int kill = 1;
        for (int i = 1; i < world_size; i++) {
            MPI_Send(&kill, 1, MPI_INT, i, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
        }
        free(batch);
    } else {
        init_queue(&task_queue);
        pthread_mutex_init(&queue_mutex, NULL);

        pthread_t listener;
        pthread_create(&listener, NULL, network_listener, NULL);

        Trade host_trades[BATCH_SIZE];
        double host_results[BATCH_SIZE];

        Trade* device_trades;
        double* device_results;
        cudaMalloc((void**)&device_trades, BATCH_SIZE * sizeof(Trade));
        cudaMalloc((void**)&device_results, BATCH_SIZE * sizeof(double));

        int processed_count = 0;

        while (simulation_running || task_queue.count > 0) {
            int items_to_process = 0;

            pthread_mutex_lock(&queue_mutex);
            while (task_queue.count > 0 && items_to_process < BATCH_SIZE) {
                host_trades[items_to_process] = pop_queue(&task_queue);
                items_to_process++;
            }
            pthread_mutex_unlock(&queue_mutex);

            if (items_to_process > 0) {
                #pragma omp parallel for
                for (int i = 0; i < items_to_process; i++) {
                    host_results[i] = 0.0;
                    host_trades[i].price += 1.5;
                }

                cudaError_t err = cudaMemcpy(device_trades, host_trades, items_to_process * sizeof(Trade), cudaMemcpyHostToDevice);
                if (err != cudaSuccess) printf("CUDA H2D error: %s\n", cudaGetErrorString(err));

                int threadsPerBlock = 256;
                int blocksPerGrid = (items_to_process + threadsPerBlock - 1) / threadsPerBlock;
                monte_carlo_risk_kernel<<<blocksPerGrid, threadsPerBlock>>>(device_trades, device_results, items_to_process);

                err = cudaDeviceSynchronize();
                if (err != cudaSuccess) printf("CUDA kernel error: %s\n", cudaGetErrorString(err));

                err = cudaMemcpy(host_results, device_results, items_to_process * sizeof(double), cudaMemcpyDeviceToHost);
                if (err != cudaSuccess) printf("CUDA D2H error: %s\n", cudaGetErrorString(err));

                processed_count += items_to_process;
                printf("[Worker %d] Processed %d trades on GPU. Total: %d\n", world_rank, items_to_process, processed_count);
            } else {
                usleep(50000);
            }
        }

        cudaFree(device_trades);
        cudaFree(device_results);
        pthread_join(listener, NULL);
        pthread_mutex_destroy(&queue_mutex);
        printf("[Worker %d] Shutting down.\n", world_rank);
    }

    MPI_Finalize();
    return 0;
}