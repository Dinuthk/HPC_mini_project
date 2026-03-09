#include "common.h"
#include "master.h"

void run_master(int world_rank, int world_size) {
    (void)world_rank;
    
    int num_workers = world_size - 1;
    
    printf("[Master] BOOTING DISTRIBUTED LOAD BALANCER SIMULATION\n");
    printf("====================================================\n");
    
    // Calculate distribution based on imbalance ratio
    int tasks_worker1 = (int)(config.total_tasks * config.imbalance_ratio / (config.imbalance_ratio + 1.0));
    int tasks_worker2 = config.total_tasks - tasks_worker1;
    
    Trade* batch = (Trade*)malloc(config.total_tasks * sizeof(Trade));
    for (int i = 0; i < config.total_tasks; i++) { 
        batch[i].stock_id = i; 
        batch[i].price = 150.0; 
        batch[i].volume = 100.0; 
    }

    printf("[Master] Creating Imbalance: Assigning %d tasks to Node 1, and %d to Node 2.\n", 
           tasks_worker1, tasks_worker2);
    MPI_Send(batch, tasks_worker1 * sizeof(Trade), MPI_BYTE, 1, TAG_WORK, MPI_COMM_WORLD);
    MPI_Send(batch, tasks_worker2 * sizeof(Trade), MPI_BYTE, 2, TAG_WORK, MPI_COMM_WORLD);

    double start_time = MPI_Wtime();
    
    // Track which workers are done
    int* workers_done = (int*)calloc(world_size, sizeof(int));
    int total_workers_done = 0;
    
    // Track steal statistics
    int total_steal_attempts = 0;
    int successful_steals = 0;
    
    while (MPI_Wtime() - start_time < config.simulation_time && total_workers_done < num_workers) {
        int flag; 
        MPI_Status status;
        
        // Check for any incoming message
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            if (status.MPI_TAG == TAG_IDLE) {
                int starving_node = status.MPI_SOURCE;
                int dummy; 
                MPI_Recv(&dummy, 1, MPI_INT, starving_node, TAG_IDLE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Find a worker that is not done
                int overloaded_node = -1;
                for (int i = 1; i < world_size; i++) {
                    if (i != starving_node && !workers_done[i]) {
                        overloaded_node = i;
                        break;
                    }
                }
                
                if (overloaded_node == -1) {
                    // No one has work, mark starving node as potentially done
                    continue;
                }
                
                printf("\n[Master] ALERT! Node %d is starving! Initiating Steal Protocol against Node %d...\n", 
                       starving_node, overloaded_node);
                
                total_steal_attempts++;
                
                int req = 1;
                MPI_Send(&req, 1, MPI_INT, overloaded_node, TAG_STEAL_REQ, MPI_COMM_WORLD);
                
                MPI_Probe(overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, &status);
                int bytes; 
                MPI_Get_count(&status, MPI_BYTE, &bytes);
                
                Trade* stolen = NULL;
                if (bytes > 0) {
                    stolen = (Trade*)malloc(bytes);
                    MPI_Recv(stolen, bytes, MPI_BYTE, overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(NULL, 0, MPI_BYTE, overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                
                int stolen_count = bytes / sizeof(Trade);
                printf("[Master] Acquired %d tasks from Node %d. Forwarding to Node %d...\n", 
                       stolen_count, overloaded_node, starving_node);
                
                if (stolen_count > 0) {
                    successful_steals++;
                    MPI_Send(stolen, bytes, MPI_BYTE, starving_node, TAG_WORK, MPI_COMM_WORLD);
                }
                
                if(stolen) free(stolen);
            }
            else if (status.MPI_TAG == TAG_WORKER_DONE) {
                int done_worker = status.MPI_SOURCE;
                int dummy;
                MPI_Recv(&dummy, 1, MPI_INT, done_worker, TAG_WORKER_DONE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                if (!workers_done[done_worker]) {
                    workers_done[done_worker] = 1;
                    total_workers_done++;
                    printf("[Master] Worker %d reported completion (%d/%d workers done)\n", 
                           done_worker, total_workers_done, num_workers);
                }
            }
        }
        usleep(20000); 
    }

    double total_time = MPI_Wtime() - start_time;

    printf("\n[Master] SIMULATION COMPLETE. Sending shutdown signals.\n");
    printf("[Master] Total steal attempts: %d, Successful: %d (%.1f%%)\n", 
           total_steal_attempts, successful_steals, 
           total_steal_attempts > 0 ? 100.0 * successful_steals / total_steal_attempts : 0.0);
    
    int kill = 1;
    for (int i = 1; i < world_size; i++) {
        MPI_Send(&kill, 1, MPI_INT, i, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
    }
    
    // Collect metrics from all workers
    WorkerMetrics* all_metrics = (WorkerMetrics*)malloc(num_workers * sizeof(WorkerMetrics));
    
    for (int i = 0; i < num_workers; i++) {
        MPI_Recv(&all_metrics[i], sizeof(WorkerMetrics), MPI_BYTE, i + 1, TAG_METRICS, 
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Export to CSV
    export_metrics_to_csv("simulation_metrics.csv", all_metrics, num_workers, total_time);
    
    // Print summary
    printf("\n====================================================\n");
    printf("SIMULATION SUMMARY\n");
    printf("====================================================\n");
    printf("Total Runtime: %.3f seconds\n", total_time);
    
    int total_processed = 0;
    for (int i = 0; i < num_workers; i++) {
        total_processed += all_metrics[i].tasks_processed;
        printf("Worker %d: %d tasks (%.1f tasks/sec), Idle: %.2f%%, Steals: %d/%d\n",
               all_metrics[i].worker_id,
               all_metrics[i].tasks_processed,
               all_metrics[i].total_time > 0 ? all_metrics[i].tasks_processed / all_metrics[i].total_time : 0.0,
               all_metrics[i].total_time > 0 ? 100.0 * all_metrics[i].idle_time / all_metrics[i].total_time : 0.0,
               all_metrics[i].steal_successes,
               all_metrics[i].steal_attempts);
    }
    printf("Total Throughput: %.1f tasks/sec\n", total_time > 0 ? total_processed / total_time : 0.0);
    printf("====================================================\n");
    
    free(all_metrics);
    free(workers_done);
    free(batch);
}