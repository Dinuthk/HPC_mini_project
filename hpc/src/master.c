#include "common.h"
#include "config.h"
#include "master.h"

void run_master(int world_rank, int world_size) {
    printf("====================================================\n");
    printf("[Master] BOOTING DISTRIBUTED LOAD BALANCER SIMULATION\n");
    printf("====================================================\n");
    
    Trade* batch = (Trade*)malloc(INITIAL_TASKS_NODE_1 * sizeof(Trade));
    for (int i = 0; i < INITIAL_TASKS_NODE_1; i++) { batch[i].stock_id = i; batch[i].price = 150.0; batch[i].volume = 100.0; }

    printf("[Master] Creating Imbalance: Assigning %d tasks to Node 1, and %d to Node 2.\n", INITIAL_TASKS_NODE_1, INITIAL_TASKS_NODE_2);
    MPI_Send(batch, INITIAL_TASKS_NODE_1 * sizeof(Trade), MPI_BYTE, 1, TAG_WORK, MPI_COMM_WORLD);
    MPI_Send(batch, INITIAL_TASKS_NODE_2 * sizeof(Trade), MPI_BYTE, 2, TAG_WORK, MPI_COMM_WORLD);

    double start_time = MPI_Wtime();
    
    while (MPI_Wtime() - start_time < SIMULATION_DURATION_SECONDS) {
        int flag; MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_IDLE, MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            int starving_node = status.MPI_SOURCE;
            int dummy; MPI_Recv(&dummy, 1, MPI_INT, starving_node, TAG_IDLE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            int overloaded_node = (starving_node == 1) ? 2 : 1;
            printf("\n[Master] ALERT! Node %d is starving! Initiating Steal Protocol against Node %d...\n", starving_node, overloaded_node);
            
            int req = 1;
            MPI_Send(&req, 1, MPI_INT, overloaded_node, TAG_STEAL_REQ, MPI_COMM_WORLD);
            
            MPI_Probe(overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, &status);
            int bytes; MPI_Get_count(&status, MPI_BYTE, &bytes);
            
            Trade* stolen = (Trade*)malloc(bytes);
            if (bytes > 0) MPI_Recv(stolen, bytes, MPI_BYTE, overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            else MPI_Recv(NULL, 0, MPI_BYTE, overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            int stolen_count = bytes / sizeof(Trade);
            printf("[Master] Acquired %d tasks from Node %d. Forwarding to Node %d...\n", stolen_count, overloaded_node, starving_node);
            
            // Only forward if there are actually tasks to send
            if (stolen_count > 0) {
                MPI_Send(stolen, bytes, MPI_BYTE, starving_node, TAG_WORK, MPI_COMM_WORLD);
            }
            if(stolen) free(stolen);
        }
        usleep(PROBE_INTERVAL_USEC); 
    }

    printf("\n[Master] SIMULATION COMPLETE. Sending shutdown signals.\n");
    int kill = 1;
    MPI_Send(&kill, 1, MPI_INT, 1, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
    MPI_Send(&kill, 1, MPI_INT, 2, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
    free(batch);
}