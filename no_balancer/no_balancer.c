#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

/**
 * NO LOAD BALANCER - Pure Sequential Version
 * 
 * NO MPI, NO OpenMP, NO Pthreads — Zero parallelism.
 * 
 * Simulates 2 workers processing tasks ONE AFTER ANOTHER
 * on a single thread. Worker 2 finishes first (100k tasks),
 * then Worker 1 continues grinding alone (500k tasks).
 * 
 * Total time = Worker 1 time + Worker 2 time (sequential)
 * 
 * Purpose: Baseline to show how much SLOWER static sequential
 *          processing is compared to the HPC load-balanced version.
 */

// Task Configuration (SAME as HPC version)
#define TASKS_WORKER_1 40000
#define TASKS_WORKER_2 6000
#define TOTAL_TASKS (TASKS_WORKER_1 + TASKS_WORKER_2)

// The Trade Struct (identical to HPC version)
typedef struct {
    int stock_id;
    double price;
    double volume;
    long timestamp;
    double complexity_weight;
} Trade;

// ---------------------------------------------------------
// CPU KERNEL: Same Monte Carlo simulation as HPC version
// But runs on a SINGLE THREAD — no OpenMP, no parallelism
// ---------------------------------------------------------
void compute_risk(Trade* trade, double* result) {
    double base_price = trade->price;
    double volume = trade->volume;
    double weight = trade->complexity_weight;
    
    double total_simulated_risk = 0.0;
    int simulations = 50000; // Same as HPC version
    
    for (int i = 0; i < simulations; i++) {
        double pseudo_rand = (double)(i % 100) / 100.0;
        double drift = (base_price * 0.02) - (volume * 0.00005);
        double volatility = sin(base_price * pseudo_rand) * cos(volume * 0.001) * weight;
        double simulated_price = base_price * exp(drift + volatility + log(1.0 + pseudo_rand));
        total_simulated_risk += simulated_price;
    }
    
    *result = total_simulated_risk / simulations;
}

// ---------------------------------------------------------
// Helper: Get current wall-clock time in seconds
// ---------------------------------------------------------
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// ---------------------------------------------------------
// Process a batch of tasks sequentially (single thread)
// ---------------------------------------------------------
double process_tasks(const char* worker_name, Trade* tasks, int num_tasks, int* out_processed) {
    int processed = 0;
    double result;
    volatile double checksum = 0.0; // volatile prevents compiler from optimizing away computation

    for (int i = 0; i < num_tasks; i++) {
        compute_risk(&tasks[i], &result);
        checksum += result; // Accumulate so compiler can't skip the math
        processed++;

        // Progress reporting every 50,000 tasks
        if (processed % 50000 == 0) {
            printf("  [%s] Progress: %d / %d tasks (%.1f%%)\n",
                   worker_name, processed, num_tasks,
                   (100.0 * processed) / num_tasks);
        }
    }

    *out_processed = processed;
    return (double)checksum;
}

// ---------------------------------------------------------
// Thread function and data
// ---------------------------------------------------------
typedef struct {
    const char* worker_name;
    Trade* tasks;
    int num_tasks;
    int processed;
    double checksum;
    double start_time;
    double end_time;
} WorkerArgs;

void* worker_thread(void* arg) {
    WorkerArgs* w = (WorkerArgs*)arg;
    w->start_time = get_time();
    w->checksum = process_tasks(w->worker_name, w->tasks, w->num_tasks, &w->processed);
    w->end_time = get_time();
    return NULL;
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║   NO LOAD BALANCER - PURE SEQUENTIAL PROCESSING           ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║                                                            ║\n");
    printf("║  Mode:             SEQUENTIAL (Single Thread)              ║\n");
    printf("║  Parallelism:      NONE (No MPI, No OpenMP, No Pthreads)  ║\n");
    printf("║                                                            ║\n");
    printf("║  Task Assignment (FIXED - No Rebalancing):                 ║\n");
    printf("║    - Worker 1: %d tasks                                ║\n", TASKS_WORKER_1);
    printf("║    - Worker 2: %d tasks                                ║\n", TASKS_WORKER_2);
    printf("║    - Total:    %d tasks                                ║\n", TOTAL_TASKS);
    printf("║                                                            ║\n");
    printf("║  Processing:       One worker at a time (sequential)       ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    // ==================== CREATE TASKS ====================
    printf("[Init] Allocating %d trades...\n", TOTAL_TASKS);
    Trade* all_tasks = (Trade*)malloc(TOTAL_TASKS * sizeof(Trade));
    if (!all_tasks) {
        printf("[Error] Failed to allocate memory for tasks.\n");
        return 1;
    }

    for (int i = 0; i < TOTAL_TASKS; i++) {
        all_tasks[i].stock_id = i;
        all_tasks[i].price = 150.0;
        all_tasks[i].volume = 100.0;
        all_tasks[i].timestamp = 0;
        all_tasks[i].complexity_weight = 1.0;
    }
    printf("[Init] Tasks created successfully.\n\n");

    // Split tasks: Worker 1 gets first 500k, Worker 2 gets last 100k
    Trade* worker1_tasks = all_tasks;
    Trade* worker2_tasks = all_tasks + TASKS_WORKER_1;

    printf("====================================================\n");
    printf("[Workers] Starting: Worker 1 (%d tasks) & Worker 2 (%d tasks) concurrently\n", TASKS_WORKER_1, TASKS_WORKER_2);
    printf("====================================================\n");

    WorkerArgs args1 = {"Worker 1", worker1_tasks, TASKS_WORKER_1, 0, 0.0, 0.0, 0.0};
    WorkerArgs args2 = {"Worker 2", worker2_tasks, TASKS_WORKER_2, 0, 0.0, 0.0, 0.0};

    pthread_t t1, t2;
    double total_start = get_time();

    pthread_create(&t1, NULL, worker_thread, &args1);
    pthread_create(&t2, NULL, worker_thread, &args2);

    pthread_join(t2, NULL);
    double w2_time = args2.end_time - args2.start_time;
    printf("[Worker 2] FINISHED. %d tasks in %.3f seconds\n", args2.processed, w2_time);
    printf("[Worker 2] Throughput: %.0f tasks/sec\n\n", args2.processed / w2_time);
    printf("[Worker 1] Worker 2 is now IDLE — no way to help!\n");

    pthread_join(t1, NULL);
    double w1_time = args1.end_time - args1.start_time;
    printf("[Worker 1] FINISHED. %d tasks in %.3f seconds\n", args1.processed, w1_time);
    printf("[Worker 1] Throughput: %.0f tasks/sec\n", args1.processed / w1_time);
    printf("[Checksum] %.2f (prevents compiler optimization)\n\n", args1.checksum + args2.checksum);

    double total_end = get_time();
    double total_time = total_end - total_start;

    // ==================== RESULTS ====================
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║   RESULTS: NO LOAD BALANCER (Sequential)                   ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║                                                            ║\n");
    printf("║  Worker 1: %-6d tasks in %8.3f sec                    ║\n", args1.processed, w1_time);
    printf("║  Worker 2: %-6d tasks in %8.3f sec                    ║\n", args2.processed, w2_time);
    printf("║                                                            ║\n");
    printf("║  Total Tasks Processed: %-6d                              ║\n", args1.processed + args2.processed);
    printf("║  Total Execution Time:  %8.3f seconds                   ║\n", total_time);
    printf("║  Overall Throughput:    %8.0f tasks/sec                  ║\n", (args1.processed + args2.processed) / total_time);
    printf("║                                                            ║\n");
    printf("║  BOTTLENECK: Worker 2 sat IDLE for %.3f seconds        ║\n", w1_time > w2_time ? (w1_time - w2_time) : 0.0);
    printf("║  while Worker 1 was still processing.                      ║\n");
    printf("║  No work stealing = wasted compute time.                   ║\n");
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    free(all_tasks);
    return 0;
}
