#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

/**
 * MULTI-THREADED VERSION - Local parallelism with pthreads
 * No HPC/MPI, uses pthread for multi-core processing
 * Simulates multiple workers without distributed memory
 */

// Task structure
typedef struct {
    int stock_id;
    double price;
    double volume;
} Trade;

// Thread-local data
typedef struct {
    int thread_id;
    int start_idx;
    int end_idx;
    Trade* tasks;
    int tasks_processed;
} ThreadData;

// Process a single task
void process_trade(Trade* trade) {
    double metric = trade->price * trade->volume;
    for (int i = 0; i < 100; i++) {
        metric = metric * 1.01;
    }
}

// Worker thread function
void* worker_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int tasks_processed = 0;
    
    printf("[Worker %d] Started - Processing tasks %d to %d\n", 
           data->thread_id, data->start_idx, data->end_idx - 1);
    
    // Process assigned tasks
    for (int i = data->start_idx; i < data->end_idx; i++) {
        process_trade(&data->tasks[i]);
        tasks_processed++;
    }
    
    data->tasks_processed = tasks_processed;
    printf("[Worker %d] Completed - Processed %d tasks\n", 
           data->thread_id, tasks_processed);
    
    return NULL;
}

int main(int argc, char** argv) {
    printf("====================================================\n");
    printf("[MULTI-THREADED] LOCAL PARALLEL TASK PROCESSING\n");
    printf("====================================================\n\n");
    
    // Configuration
    int total_tasks = 90000;  // Same as HPC version
    int num_threads = 2;      // Match HPC workers (2 workers)
    
    printf("[Info] Starting task processing...\n");
    printf("[Info] Total Tasks: %d\n", total_tasks);
    printf("[Info] Number of Threads: %d\n", num_threads);
    printf("[Info] Mode: MULTI-THREADED (Local parallelism)\n\n");
    
    // Start timing
    double start_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Create task batch
    Trade* tasks = (Trade*)malloc(total_tasks * sizeof(Trade));
    
    // Initialize tasks
    printf("[Initialization] Creating %d tasks...\n", total_tasks);
    for (int i = 0; i < total_tasks; i++) {
        tasks[i].stock_id = i;
        tasks[i].price = 150.0 + (i % 100) * 0.01;
        tasks[i].volume = 100.0 + (i % 50);
    }
    
    double init_time = (double)clock() / CLOCKS_PER_SEC;
    printf("[Initialization Complete] Time: %.3f seconds\n\n", init_time - start_time);
    
    // Create thread data structures
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    ThreadData* thread_data = (ThreadData*)malloc(num_threads * sizeof(ThreadData));
    
    // Distribute tasks among threads
    printf("[Distribution] Distributing %d tasks among %d threads...\n", total_tasks, num_threads);
    int tasks_per_thread = total_tasks / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i + 1;
        thread_data[i].start_idx = i * tasks_per_thread;
        thread_data[i].end_idx = (i == num_threads - 1) ? total_tasks : (i + 1) * tasks_per_thread;
        thread_data[i].tasks = tasks;
        thread_data[i].tasks_processed = 0;
        
        printf("  Thread %d: tasks %d-%d (%d tasks)\n", 
               i + 1, thread_data[i].start_idx, thread_data[i].end_idx - 1,
               thread_data[i].end_idx - thread_data[i].start_idx);
    }
    
    // Create threads
    printf("\n[Threading] Creating %d worker threads...\n", num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, &thread_data[i]);
    }
    
    // Wait for all threads to complete
    printf("[Threading] Waiting for threads to complete...\n\n");
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double processing_time = (double)clock() / CLOCKS_PER_SEC;
    
    // Calculate total processed
    int total_processed = 0;
    for (int i = 0; i < num_threads; i++) {
        total_processed += thread_data[i].tasks_processed;
    }
    
    printf("[Processing Complete] Tasks Processed: %d\n", total_processed);
    printf("[Processing Time] %.3f seconds\n\n", processing_time - init_time);
    
    // Cleanup
    free(threads);
    free(thread_data);
    free(tasks);
    
    // Summary
    double total_time = (double)clock() / CLOCKS_PER_SEC;
    printf("====================================================\n");
    printf("[SUMMARY] MULTI-THREADED EXECUTION\n");
    printf("====================================================\n");
    printf("Initialization Time:  %.3f seconds\n", init_time - start_time);
    printf("Processing Time:      %.3f seconds\n", processing_time - init_time);
    printf("Total Execution Time: %.3f seconds\n", total_time - start_time);
    printf("Tasks Processed:      %d\n", total_processed);
    printf("Tasks/Second:         %.0f\n\n", total_processed / (processing_time - init_time + 0.001));
    
    return 0;
}
