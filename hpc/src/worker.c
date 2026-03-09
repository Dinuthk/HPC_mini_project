#include "common.h"
#include "worker.h"
#include "listener.h"
#include "compute.h"

void run_worker(int world_rank, int world_size) {
    (void)world_size;
    init_queue(&task_queue);
    pthread_mutex_init(&queue_mutex, NULL);
    
    // Initialize worker metrics
    worker_metrics.worker_id = world_rank;
    worker_metrics.tasks_processed = 0;
    worker_metrics.steal_attempts = 0;
    worker_metrics.steal_successes = 0;
    worker_metrics.compute_time = 0.0;
    worker_metrics.idle_time = 0.0;
    
    double worker_start_time = get_wall_time();
    
    pthread_t listener;
    pthread_create(&listener, NULL, network_listener, NULL);

    Trade* cpu_batch_trades = (Trade*)malloc(config.batch_size * sizeof(Trade));
    double* cpu_batch_results = (double*)malloc(config.batch_size * sizeof(double));
    
    int consecutive_idle_checks = 0;
    int work_done_signaled = 0;

    while (simulation_running || task_queue.count > 0) {
        int items_to_process = 0;

        pthread_mutex_lock(&queue_mutex);
        
        if (task_queue.count < config.low_watermark && !waiting_for_work && simulation_running) {
            int msg = 1;
            MPI_Send(&msg, 1, MPI_INT, 0, TAG_IDLE, MPI_COMM_WORLD);
            waiting_for_work = 1; 
            worker_metrics.steal_attempts++;
        }

        while (task_queue.count > 0 && items_to_process < config.batch_size) {
            cpu_batch_trades[items_to_process] = pop_front_queue(&task_queue);
            items_to_process++;
        }
        
        // Check if we're truly idle (no work and simulation ending)
        if (task_queue.count == 0 && !simulation_running) {
            consecutive_idle_checks++;
        } else if (items_to_process > 0) {
            consecutive_idle_checks = 0;
        }
        
        pthread_mutex_unlock(&queue_mutex);

        if (items_to_process > 0) {
            double compute_start = get_wall_time();
            compute_risk_batch(cpu_batch_trades, cpu_batch_results, items_to_process);
            double compute_end = get_wall_time();
            
            worker_metrics.tasks_processed += items_to_process;
            worker_metrics.compute_time += (compute_end - compute_start);
        } else {
            // Idle time
            double idle_start = get_wall_time();
            usleep(50000);
            double idle_end = get_wall_time();
            worker_metrics.idle_time += (idle_end - idle_start);
            
            // If we've been idle for a while and simulation is ending, signal we're done
            if (consecutive_idle_checks > 5 && !work_done_signaled && !simulation_running) {
                int done_msg = 1;
                MPI_Send(&done_msg, 1, MPI_INT, 0, TAG_WORKER_DONE, MPI_COMM_WORLD);
                work_done_signaled = 1;
            }
        }
    }

    double worker_end_time = get_wall_time();
    worker_metrics.total_time = worker_end_time - worker_start_time;

    pthread_join(listener, NULL);
    pthread_mutex_destroy(&queue_mutex);
    
    printf("[Worker %d] Shutdown successful. Processed a total of %d trades.\n", 
           world_rank, worker_metrics.tasks_processed);
    
    // Send metrics to master
    MPI_Send(&worker_metrics, sizeof(WorkerMetrics), MPI_BYTE, 0, TAG_METRICS, MPI_COMM_WORLD);
    
    free(cpu_batch_trades);
    free(cpu_batch_results);
}