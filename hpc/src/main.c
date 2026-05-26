#include "common.h"
#include "config.h"
#include "master.h"
#include "worker.h"

// Define Global Variables exactly once
TradeQueue task_queue;
pthread_mutex_t queue_mutex;
int simulation_running = 1;
int waiting_for_work = 0;

// Queue Implementations
void init_queue(TradeQueue* q) { q->front = 0; q->rear = -1; q->count = 0; }

void push_queue(TradeQueue* q, Trade t) {
    if (q->count < MAX_QUEUE_SIZE) {
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        q->data[q->rear] = t;
        q->count++;
    }
}

Trade pop_front_queue(TradeQueue* q) {
    Trade t = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->count--;
    return t;
}

Trade pop_rear_queue(TradeQueue* q) {
    Trade t = q->data[q->rear];
    q->rear = (q->rear - 1 + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
    q->count--;
    return t;
}

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    // Get the rank (ID) of the current process (0 for Master, 1/2 for Workers)
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    // Get the total number of processes in this communicator
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Ensure we have at least 1 Master and 2 Workers
    if (world_size < 3) {
        if (world_rank == 0) printf("[Error] Need at least 3 nodes.\n");
        MPI_Finalize();
        return 1;
    }

    // Display system configuration
    display_system_config(world_rank, world_size);

    // Split execution based on rank
    if (world_rank == 0) {
        // Rank 0 is always the Master orchestrator
        run_master(world_rank, world_size);
    } else {
        // All other ranks act as Workers
        run_worker(world_rank, world_size);
    }

    // Clean up and shutdown MPI environment
    MPI_Finalize();
    return 0;
}