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
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size < 3) {
        if (world_rank == 0) printf("[Error] Need at least 3 nodes.\n");
        MPI_Finalize();
        return 1;
    }

    // Display system configuration
    display_system_config(world_rank, world_size);

    if (world_rank == 0) {
        run_master(world_rank, world_size);
    } else {
        run_worker(world_rank, world_size);
    }

    MPI_Finalize();
    return 0;
}