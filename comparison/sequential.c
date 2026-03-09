#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/**
 * SEQUENTIAL VERSION - Single-threaded task processing
 * No HPC/MPI, no threading
 * Simple loop-based task execution
 */

// Define task structure
typedef struct {
    int stock_id;
    double price;
    double volume;
} Trade;

// Process a single task (simulated work)
void process_trade(Trade* trade) {
    // Simulate computation: calculate some metric
    double metric = trade->price * trade->volume;
    // Simulate some work with loop
    for (int i = 0; i < 100; i++) {
        metric = metric * 1.01;  // Simple calculation
    }
}

int main(int argc, char** argv) {
    printf("====================================================\n");
    printf("[SEQUENTIAL] SINGLE-THREADED TASK PROCESSING\n");
    printf("====================================================\n\n");
    
    // Configuration
    int total_tasks = 90000;  // Same as HPC version (80k + 10k)
    
    printf("[Info] Starting task processing...\n");
    printf("[Info] Total Tasks: %d\n", total_tasks);
    printf("[Info] Mode: SEQUENTIAL (Single-threaded)\n\n");
    
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
    
    // Process all tasks sequentially
    printf("[Processing] Starting sequential task execution...\n");
    int tasks_processed = 0;
    
    for (int i = 0; i < total_tasks; i++) {
        process_trade(&tasks[i]);
        tasks_processed++;
        
        // Print progress
        if ((i + 1) % 10000 == 0) {
            printf("  Progress: %d/%d tasks processed\n", i + 1, total_tasks);
        }
    }
    
    double processing_time = (double)clock() / CLOCKS_PER_SEC;
    printf("[Processing Complete] Tasks Processed: %d\n", tasks_processed);
    printf("[Processing Time] %.3f seconds\n\n", processing_time - init_time);
    
    // Cleanup
    free(tasks);
    
    // Summary
    double total_time = (double)clock() / CLOCKS_PER_SEC;
    printf("====================================================\n");
    printf("[SUMMARY] SEQUENTIAL EXECUTION\n");
    printf("====================================================\n");
    printf("Initialization Time:  %.3f seconds\n", init_time - start_time);
    printf("Processing Time:      %.3f seconds\n", processing_time - init_time);
    printf("Total Execution Time: %.3f seconds\n", total_time - start_time);
    printf("Tasks Processed:      %d\n", tasks_processed);
    printf("Tasks/Second:         %.0f\n\n", tasks_processed / (processing_time - init_time + 0.001));
    
    return 0;
}
