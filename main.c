#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define TAG_WORK 1
#define TAG_KILL_SIGNAL 2
#define MAX_QUEUE_SIZE 200000 // Large enough to hold our batches

// 1. The Trade Struct
typedef struct {
    int stock_id;
    double price;
    double volume;
    long timestamp;
    double complexity_weight;
} Trade;

// 2. The C-Equivalent of std::queue<Trade>
typedef struct {
    Trade data[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
} TradeQueue;

void init_queue(TradeQueue* q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

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

// Shared Globals for the Worker Threads
TradeQueue task_queue;
pthread_mutex_t queue_mutex;
int simulation_running = 1;

// ---------------------------------------------------------
// BACKGROUND THREAD: The Asynchronous Listener
// ---------------------------------------------------------
void* network_listener(void* arg) {
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    while (simulation_running) {
        int flag = 0;
        MPI_Status status;
        
        // Non-blocking check for incoming messages
        MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            if (status.MPI_TAG == TAG_WORK) {
                // Find out exactly how many bytes (and trades) are arriving
                int incoming_bytes;
                MPI_Get_count(&status, MPI_BYTE, &incoming_bytes);
                int num_trades = incoming_bytes / sizeof(Trade);

                // Allocate temporary memory to receive the batch
                Trade* incoming_batch = (Trade*)malloc(incoming_bytes);
                MPI_Recv(incoming_batch, incoming_bytes, MPI_BYTE, 0, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Safely lock the mutex and push the new trades into our queue
                pthread_mutex_lock(&queue_mutex);
                for (int i = 0; i < num_trades; i++) {
                    push_queue(&task_queue, incoming_batch[i]);
                }
                printf("\n[Worker %d Listener] INTERRUPT: Received %d new trades! Queue size now: %d\n", 
                       world_rank, num_trades, task_queue.count);
                pthread_mutex_unlock(&queue_mutex);

                free(incoming_batch);
            } 
            else if (status.MPI_TAG == TAG_KILL_SIGNAL) {
                int kill_msg;
                MPI_Recv(&kill_msg, 1, MPI_INT, 0, TAG_KILL_SIGNAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                simulation_running = 0; 
            }
        }
        usleep(10000); // Sleep 10ms to prevent the listener from burning 100% CPU
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

    if (world_size < 3) {
        if (world_rank == 0) printf("[Error] Need 3 nodes.\n");
        MPI_Finalize();
        return 1;
    }

    // ---------------- MASTER NODE ----------------
    if (world_rank == 0) {
        printf("[Master] Booting market simulation...\n");
        
        Trade* batch = (Trade*)malloc(50000 * sizeof(Trade));
        for (int i = 0; i < 50000; i++) batch[i].stock_id = i; // Dummy data

        printf("[Master] Sending initial morning trades (50,000 each)...\n");
        MPI_Send(batch, 50000 * sizeof(Trade), MPI_BYTE, 1, TAG_WORK, MPI_COMM_WORLD);
        MPI_Send(batch, 50000 * sizeof(Trade), MPI_BYTE, 2, TAG_WORK, MPI_COMM_WORLD);

        // Wait a few seconds while workers are busy processing
        sleep(2); 

        printf("\n[Master] MARKET EVENT! Tech volume spike detected. Sending 10,000 extra trades to Worker 1...\n");
        MPI_Send(batch, 10000 * sizeof(Trade), MPI_BYTE, 1, TAG_WORK, MPI_COMM_WORLD);

        sleep(3);
        printf("\n[Master] End of trading day. Sending kill signals.\n");
        int kill = 1;
        MPI_Send(&kill, 1, MPI_INT, 1, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
        MPI_Send(&kill, 1, MPI_INT, 2, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
        
        free(batch);
    } 
    
    // ---------------- WORKER NODES ----------------
    else {
        init_queue(&task_queue);
        pthread_mutex_init(&queue_mutex, NULL);
        
        // Launch the background listener thread
        pthread_t listener;
        pthread_create(&listener, NULL, network_listener, NULL);

        int processed_count = 0;

        // MAIN CPU THREAD: Processing the queue
        while (simulation_running || task_queue.count > 0) {
            int has_work = 0;
            Trade current_task;

            // Safely check and pop from the queue
            pthread_mutex_lock(&queue_mutex);
            if (task_queue.count > 0) {
                current_task = pop_queue(&task_queue);
                has_work = 1;
            }
            pthread_mutex_unlock(&queue_mutex);

            if (has_work) {
                // Simulate processing time
                processed_count++;
                
                // Print an update every 10,000 trades so we don't spam the terminal
                if (processed_count % 10000 == 0) {
                    printf("[Worker %d Main] Processed %d trades so far... (Queue remaining: %d)\n", 
                           world_rank, processed_count, task_queue.count);
                }
            } else {
                usleep(50000); // Idle, waiting for new work
            }
        }

        pthread_join(listener, NULL);
        pthread_mutex_destroy(&queue_mutex);
        printf("[Worker %d] Shutting down. Total trades processed today: %d\n", world_rank, processed_count);
    }

    MPI_Finalize();
    return 0;
}