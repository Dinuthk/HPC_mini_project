#include "common.h"
#include "worker.h"
#include "listener.h"
#include "compute.h"

void run_worker(int world_rank, int world_size) {
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
        
        if (task_queue.count < LOW_WATERMARK && !waiting_for_work && simulation_running) {
            int msg = 1;
            MPI_Send(&msg, 1, MPI_INT, 0, TAG_IDLE, MPI_COMM_WORLD);
            waiting_for_work = 1; 
        }

        while (task_queue.count > 0 && items_to_process < BATCH_SIZE) {
            cpu_batch_trades[items_to_process] = pop_front_queue(&task_queue);
            items_to_process++;
        }
        pthread_mutex_unlock(&queue_mutex);

        if (items_to_process > 0) {
            compute_risk_batch(cpu_batch_trades, cpu_batch_results, items_to_process);
            processed_count += items_to_process;
        } else {
            usleep(50000); 
        }
    }

    pthread_join(listener, NULL);
    pthread_mutex_destroy(&queue_mutex);
    printf("[Worker %d] Shutdown successful. Processed a total of %d trades today.\n", world_rank, processed_count);
}