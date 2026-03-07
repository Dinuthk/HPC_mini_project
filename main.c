// #include <mpi.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <pthread.h>
// #include <unistd.h>
// #include <omp.h>

// #define TAG_WORK 1
// #define TAG_KILL_SIGNAL 2
// #define MAX_QUEUE_SIZE 200000
// #define BATCH_SIZE 1024 

// // 1. The Trade Struct
// typedef struct {
//     int stock_id;
//     double price;
//     double volume;
//     long timestamp;
//     double complexity_weight;
// } Trade;

// // 2. Thread-Safe Queue
// typedef struct {
//     Trade data[MAX_QUEUE_SIZE];
//     int front;
//     int rear;
//     int count;
// } TradeQueue;

// void init_queue(TradeQueue* q) { q->front = 0; q->rear = -1; q->count = 0; }

// void push_queue(TradeQueue* q, Trade t) {
//     if (q->count < MAX_QUEUE_SIZE) {
//         q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
//         q->data[q->rear] = t;
//         q->count++;
//     }
// }

// Trade pop_queue(TradeQueue* q) {
//     Trade t = q->data[q->front];
//     q->front = (q->front + 1) % MAX_QUEUE_SIZE;
//     q->count--;
//     return t;
// }

// // Shared Globals
// TradeQueue task_queue;
// pthread_mutex_t queue_mutex;
// int simulation_running = 1;

// // ---------------------------------------------------------
// // CPU KERNEL: Mock Monte Carlo Risk Simulation (OpenMP)
// // ---------------------------------------------------------
// void compute_risk_batch(Trade* batch_trades, double* batch_results, int num_trades) {
//     // This pragma tells your AMD Ryzen to use ALL of its CPU cores to run this loop
//     #pragma omp parallel for
//     for (int idx = 0; idx < num_trades; idx++) {
//         double price = batch_trades[idx].price;
//         double vol = batch_trades[idx].volume;
//         double risk = 0.0;

//         // Mock heavy calculation (Simulating 1000 Monte Carlo paths)
//         for (int i = 0; i < 1000; i++) {
//             risk += (price * 0.01) + (vol * 0.0001) - (i * 0.001); 
//         }
        
//         batch_results[idx] = risk;
//     }
// }

// // ---------------------------------------------------------
// // BACKGROUND THREAD: The Asynchronous Listener
// // ---------------------------------------------------------
// void* network_listener(void* arg) {
//     int world_rank;
//     MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

//     while (simulation_running) {
//         int flag = 0;
//         MPI_Status status;
//         MPI_Iprobe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        
//         if (flag) {
//             if (status.MPI_TAG == TAG_WORK) {
//                 int incoming_bytes;
//                 MPI_Get_count(&status, MPI_BYTE, &incoming_bytes);
//                 int num_trades = incoming_bytes / sizeof(Trade);

//                 Trade* incoming_batch = (Trade*)malloc(incoming_bytes);
//                 MPI_Recv(incoming_batch, incoming_bytes, MPI_BYTE, 0, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
//                 pthread_mutex_lock(&queue_mutex);
//                 for (int i = 0; i < num_trades; i++) push_queue(&task_queue, incoming_batch[i]);
//                 printf("\n[Worker %d Listener] INTERRUPT: Received %d new trades! Queue: %d\n", world_rank, num_trades, task_queue.count);
//                 pthread_mutex_unlock(&queue_mutex);

//                 free(incoming_batch);
//             } 
//             else if (status.MPI_TAG == TAG_KILL_SIGNAL) {
//                 int kill_msg;
//                 MPI_Recv(&kill_msg, 1, MPI_INT, 0, TAG_KILL_SIGNAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//                 simulation_running = 0; 
//             }
//         }
//         usleep(10000); 
//     }
//     return NULL;
// }

// // ---------------------------------------------------------
// // MAIN EXECUTABLE
// // ---------------------------------------------------------
// int main(int argc, char** argv) {
//     MPI_Init(&argc, &argv);

//     int world_rank, world_size;
//     MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
//     MPI_Comm_size(MPI_COMM_WORLD, &world_size);

//     if (world_rank == 0) {
//         printf("[Master] Booting market simulation...\n");
//         Trade* batch = (Trade*)malloc(50000 * sizeof(Trade));
//         for (int i = 0; i < 50000; i++) { batch[i].stock_id = i; batch[i].price = 100.0; batch[i].volume = 10.0; }

//         printf("[Master] Sending morning trades (50,000 each)...\n");
//         MPI_Send(batch, 50000 * sizeof(Trade), MPI_BYTE, 1, TAG_WORK, MPI_COMM_WORLD);
//         MPI_Send(batch, 50000 * sizeof(Trade), MPI_BYTE, 2, TAG_WORK, MPI_COMM_WORLD);

//         sleep(2); 
//         int kill = 1;
//         MPI_Send(&kill, 1, MPI_INT, 1, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
//         MPI_Send(&kill, 1, MPI_INT, 2, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
//         free(batch);
//     } 
//     else {
//         init_queue(&task_queue);
//         pthread_mutex_init(&queue_mutex, NULL);
        
//         pthread_t listener;
//         pthread_create(&listener, NULL, network_listener, NULL);

//         // Memory buffers for the batch
//         Trade cpu_batch_trades[BATCH_SIZE];
//         double cpu_batch_results[BATCH_SIZE];

//         int processed_count = 0;

//         // Print how many Ryzen threads we are about to use
//         #pragma omp parallel
//         {
//             #pragma omp single
//             printf("[Worker %d] OpenMP Engine Online. Max Threads available: %d\n", world_rank, omp_get_num_threads());
//         }

//         while (simulation_running || task_queue.count > 0) {
//             int items_to_process = 0;

//             // 1. SAFELY POP A BATCH FROM THE QUEUE
//             pthread_mutex_lock(&queue_mutex);
//             while (task_queue.count > 0 && items_to_process < BATCH_SIZE) {
//                 cpu_batch_trades[items_to_process] = pop_queue(&task_queue);
//                 items_to_process++;
//             }
//             pthread_mutex_unlock(&queue_mutex);

//             if (items_to_process > 0) {
//                 // 2. OPENMP: Fire up the Ryzen CPU cores to do the heavy math
//                 compute_risk_batch(cpu_batch_trades, cpu_batch_results, items_to_process);

//                 processed_count += items_to_process;
                
//                 // Only print occasionally so we don't spam the terminal
//                 if (processed_count % (BATCH_SIZE * 10) < BATCH_SIZE) {
//                      printf("[Worker %d Main] Processed batch of %d trades using OpenMP. Total: %d\n", world_rank, items_to_process, processed_count);
//                 }
//             } else {
//                 usleep(50000); 
//             }
//         }

//         pthread_join(listener, NULL);
//         pthread_mutex_destroy(&queue_mutex);
//         printf("[Worker %d] Shutting down successfully.\n", world_rank);
//     }

//     MPI_Finalize();
//     return 0;
// }

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <omp.h>

// Extended Communication Protocol Tags
#define TAG_WORK 1
#define TAG_KILL_SIGNAL 2
#define TAG_IDLE 3          // Worker telling Master it needs work
#define TAG_STEAL_REQ 4     // Master ordering a Worker to surrender tasks
#define TAG_STOLEN_WORK 5   // Overloaded Worker sending tasks back to Master

#define MAX_QUEUE_SIZE 200000
#define BATCH_SIZE 1024 
#define LOW_WATERMARK 1000  // If queue drops below this, ask for help

// 1. The Trade Struct
typedef struct {
    int stock_id;
    double price;
    double volume;
    long timestamp;
    double complexity_weight;
} Trade;

// 2. Thread-Safe Queue
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

// Popping from the front (Used by the OpenMP CPU workers)
Trade pop_front_queue(TradeQueue* q) {
    Trade t = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->count--;
    return t;
}

// NEW: Popping from the rear (Used by the Pthread listener to steal work safely)
Trade pop_rear_queue(TradeQueue* q) {
    Trade t = q->data[q->rear];
    q->rear = (q->rear - 1 + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
    q->count--;
    return t;
}

// Shared Globals
TradeQueue task_queue;
pthread_mutex_t queue_mutex;
int simulation_running = 1;
int waiting_for_work = 0; // Prevents spamming the Master with requests

// ---------------------------------------------------------
// CPU KERNEL: Mock Monte Carlo Risk Simulation (OpenMP)
// ---------------------------------------------------------
void compute_risk_batch(Trade* batch_trades, double* batch_results, int num_trades) {
    #pragma omp parallel for
    for (int idx = 0; idx < num_trades; idx++) {
        double risk = 0.0;
        for (int i = 0; i < 1000; i++) {
            risk += (batch_trades[idx].price * 0.01) + (batch_trades[idx].volume * 0.0001) - (i * 0.001); 
        }
        batch_results[idx] = risk;
    }
}

// ---------------------------------------------------------
// BACKGROUND THREAD: The Orchestrator (Pthreads)
// ---------------------------------------------------------
void* network_listener(void* arg) {
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    while (simulation_running) {
        int flag = 0;
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            // SCENARIO A: We are receiving new work
            if (status.MPI_TAG == TAG_WORK) {
                int incoming_bytes;
                MPI_Get_count(&status, MPI_BYTE, &incoming_bytes);
                int num_trades = incoming_bytes / sizeof(Trade);

                Trade* incoming_batch = (Trade*)malloc(incoming_bytes);
                MPI_Recv(incoming_batch, incoming_bytes, MPI_BYTE, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                pthread_mutex_lock(&queue_mutex);
                for (int i = 0; i < num_trades; i++) push_queue(&task_queue, incoming_batch[i]);
                waiting_for_work = 0; // Reset our starvation flag
                printf("\n[Worker %d Listener] SUCCESS! Received %d trades. Queue revitalized to: %d\n", world_rank, num_trades, task_queue.count);
                pthread_mutex_unlock(&queue_mutex);

                free(incoming_batch);
            } 
            
            // SCENARIO B: Master is ordering us to surrender tasks to save another node
            else if (status.MPI_TAG == TAG_STEAL_REQ) {
                int req;
                MPI_Recv(&req, 1, MPI_INT, 0, TAG_STEAL_REQ, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                pthread_mutex_lock(&queue_mutex);
                int steal_count = task_queue.count / 2; // Surrender exactly half our remaining work
                Trade* stolen = NULL;
                
                if (steal_count > 0) {
                    stolen = (Trade*)malloc(steal_count * sizeof(Trade));
                    // We pop from the REAR so we don't interfere with the CPU processing the front
                    for (int i = 0; i < steal_count; i++) {
                        stolen[i] = pop_rear_queue(&task_queue);
                    }
                }
                pthread_mutex_unlock(&queue_mutex);

                // Send the stolen data back to the Master router
                MPI_Send(stolen, steal_count * sizeof(Trade), MPI_BYTE, 0, TAG_STOLEN_WORK, MPI_COMM_WORLD);
                printf("\n[Worker %d Listener] STEAL EXECUTED! Surrendered %d tasks from the rear of my queue.\n", world_rank, steal_count);
                if (stolen) free(stolen);
            }

            // SCENARIO C: Market Closed
            else if (status.MPI_TAG == TAG_KILL_SIGNAL) {
                int kill_msg;
                MPI_Recv(&kill_msg, 1, MPI_INT, 0, TAG_KILL_SIGNAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                simulation_running = 0; 
            }
        }
        usleep(5000); // 5ms sleep loop
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

    // ---------------- MASTER NODE (The Traffic Controller) ----------------
    if (world_rank == 0) {
        printf("====================================================\n");
        printf("[Master] BOOTING DISTRIBUTED LOAD BALANCER SIMULATION\n");
        printf("====================================================\n");
        
        Trade* batch = (Trade*)malloc(80000 * sizeof(Trade));
        for (int i = 0; i < 80000; i++) { batch[i].stock_id = i; batch[i].price = 150.0; batch[i].volume = 100.0; }

        // FORCE IMBALANCE: Send 80k to Node 1, only 10k to Node 2
        printf("[Master] Creating Imbalance: Assigning 80,000 tasks to Node 1, and 10,000 to Node 2.\n");
        MPI_Send(batch, 80000 * sizeof(Trade), MPI_BYTE, 1, TAG_WORK, MPI_COMM_WORLD);
        MPI_Send(batch, 10000 * sizeof(Trade), MPI_BYTE, 2, TAG_WORK, MPI_COMM_WORLD);

        double start_time = MPI_Wtime();
        
        // Orchestration Loop: Run the market for 6 seconds
        while (MPI_Wtime() - start_time < 6.0) {
            int flag; MPI_Status status;
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_IDLE, MPI_COMM_WORLD, &flag, &status);
            
            if (flag) {
                int starving_node = status.MPI_SOURCE;
                int dummy; MPI_Recv(&dummy, 1, MPI_INT, starving_node, TAG_IDLE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                int overloaded_node = (starving_node == 1) ? 2 : 1;
                printf("\n[Master] ALERT! Node %d is starving! Initiating Steal Protocol against Node %d...\n", starving_node, overloaded_node);
                
                // Order the overloaded node to surrender data
                int req = 1;
                MPI_Send(&req, 1, MPI_INT, overloaded_node, TAG_STEAL_REQ, MPI_COMM_WORLD);
                
                // Wait to receive the stolen data
                MPI_Probe(overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, &status);
                int bytes; MPI_Get_count(&status, MPI_BYTE, &bytes);
                
                Trade* stolen = (Trade*)malloc(bytes);
                if (bytes > 0) MPI_Recv(stolen, bytes, MPI_BYTE, overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                else MPI_Recv(NULL, 0, MPI_BYTE, overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                int stolen_count = bytes / sizeof(Trade);
                printf("[Master] Successfully acquired %d tasks from Node %d. Forwarding to Node %d...\n", stolen_count, overloaded_node, starving_node);
                
                // Give the stolen data to the starving node
                MPI_Send(stolen, bytes, MPI_BYTE, starving_node, TAG_WORK, MPI_COMM_WORLD);
                if(stolen) free(stolen);
            }
            usleep(20000); 
        }

        printf("\n[Master] SIMULATION COMPLETE. Sending shutdown signals.\n");
        int kill = 1;
        MPI_Send(&kill, 1, MPI_INT, 1, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
        MPI_Send(&kill, 1, MPI_INT, 2, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
        free(batch);
    } 
    
    // ---------------- WORKER NODES (The Compute Engines) ----------------
    else {
        init_queue(&task_queue);
        pthread_mutex_init(&queue_mutex, NULL);
        
        pthread_t listener;
        pthread_create(&listener, NULL, network_listener, NULL);

        Trade cpu_batch_trades[BATCH_SIZE];
        double cpu_batch_results[BATCH_SIZE];
        int processed_count = 0;

        while (simulation_running || task_queue.count > 0) {
            int items_to_process = 0;

            pthread_mutex_lock(&queue_mutex);
            
            // DYNAMIC WORK STEALING LOGIC: Should I ask for help?
            if (task_queue.count < LOW_WATERMARK && !waiting_for_work && simulation_running) {
                int msg = 1;
                MPI_Send(&msg, 1, MPI_INT, 0, TAG_IDLE, MPI_COMM_WORLD);
                waiting_for_work = 1; 
            }

            // Grab a batch of data from the FRONT of the queue
            while (task_queue.count > 0 && items_to_process < BATCH_SIZE) {
                cpu_batch_trades[items_to_process] = pop_front_queue(&task_queue);
                items_to_process++;
            }
            pthread_mutex_unlock(&queue_mutex);

            // Execute the OpenMP Heavy Math
            if (items_to_process > 0) {
                compute_risk_batch(cpu_batch_trades, cpu_batch_results, items_to_process);
                processed_count += items_to_process;
            } else {
                usleep(50000); // Idle, waiting for stolen tasks to arrive
            }
        }

        pthread_join(listener, NULL);
        pthread_mutex_destroy(&queue_mutex);
        printf("[Worker %d] Shutdown successful. Processed a total of %d trades today.\n", world_rank, processed_count);
    }

    MPI_Finalize();
    return 0;
}