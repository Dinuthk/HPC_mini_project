#include "common.h"
#include "listener.h"

void* network_listener(void* arg) {
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    while (simulation_running) {
        int flag = 0;
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            if (status.MPI_TAG == TAG_WORK) {
                int incoming_bytes;
                MPI_Get_count(&status, MPI_BYTE, &incoming_bytes);
                int num_trades = incoming_bytes / sizeof(Trade);

                Trade* incoming_batch = (Trade*)malloc(incoming_bytes);
                MPI_Recv(incoming_batch, incoming_bytes, MPI_BYTE, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                pthread_mutex_lock(&queue_mutex);
                for (int i = 0; i < num_trades; i++) push_queue(&task_queue, incoming_batch[i]);
                waiting_for_work = 0; 
                printf("\n[Worker %d Listener] SUCCESS! Received %d trades. Queue revitalized to: %d\n", world_rank, num_trades, task_queue.count);
                pthread_mutex_unlock(&queue_mutex);

                free(incoming_batch);
            } 
            else if (status.MPI_TAG == TAG_STEAL_REQ) {
                int req;
                MPI_Recv(&req, 1, MPI_INT, 0, TAG_STEAL_REQ, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                pthread_mutex_lock(&queue_mutex);
                int steal_count = task_queue.count / 2; 
                Trade* stolen = NULL;
                
                if (steal_count > 0) {
                    stolen = (Trade*)malloc(steal_count * sizeof(Trade));
                    for (int i = 0; i < steal_count; i++) {
                        stolen[i] = pop_rear_queue(&task_queue);
                    }
                }
                pthread_mutex_unlock(&queue_mutex);

                MPI_Send(stolen, steal_count * sizeof(Trade), MPI_BYTE, 0, TAG_STOLEN_WORK, MPI_COMM_WORLD);
                printf("\n[Worker %d Listener] STEAL EXECUTED! Surrendered %d tasks from the rear of my queue.\n", world_rank, steal_count);
                if (stolen) free(stolen);
            }
            else if (status.MPI_TAG == TAG_KILL_SIGNAL) {
                int kill_msg;
                MPI_Recv(&kill_msg, 1, MPI_INT, 0, TAG_KILL_SIGNAL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                simulation_running = 0; 
            }
        }
        usleep(5000); 
    }
    return NULL;
}