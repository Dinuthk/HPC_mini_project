#include "common.h"
#include "config.h"
#include "master.h"

void run_master(int world_rank, int world_size) {
    printf("====================================================\n");
    printf("[Master] BOOTING DISTRIBUTED LOAD BALANCER SIMULATION\n");
    printf("====================================================\n");
    
    Trade* batch = (Trade*)malloc(INITIAL_TASKS_NODE_1 * sizeof(Trade));
    for (int i = 0; i < INITIAL_TASKS_NODE_1; i++) { batch[i].stock_id = i; batch[i].price = 150.0; batch[i].volume = 100.0; }

    double start_time = MPI_Wtime();

    printf("[Master] Creating Imbalance: Assigning %d tasks to Node 1, and %d to Node 2.\n", INITIAL_TASKS_NODE_1, INITIAL_TASKS_NODE_2);
    MPI_Send(batch, INITIAL_TASKS_NODE_1 * sizeof(Trade), MPI_BYTE, 1, TAG_WORK, MPI_COMM_WORLD);
    MPI_Send(batch, INITIAL_TASKS_NODE_2 * sizeof(Trade), MPI_BYTE, 2, TAG_WORK, MPI_COMM_WORLD);

    int all_idle_count = 0; // Track consecutive empty steals for early termination

    // Main simulation loop: run until duration expires
    while (MPI_Wtime() - start_time < SIMULATION_DURATION_SECONDS) {
        int flag; MPI_Status status;
        // Non-blocking check to see if any worker sent a TAG_IDLE (starving) message
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_IDLE, MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            // A worker is out of tasks!
            int starving_node = status.MPI_SOURCE; // Identify who is starving
            int dummy; 
            // Receive the dummy message to clear it from the queue
            MPI_Recv(&dummy, 1, MPI_INT, starving_node, TAG_IDLE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Determine who to steal from (if 1 is starving, steal from 2, and vice versa)
            int overloaded_node = (starving_node == 1) ? 2 : 1;
            printf("\n[Master] ALERT! Node %d is starving! Initiating Steal Protocol against Node %d...\n", starving_node, overloaded_node);
            
            int req = 1;
            // Send a steal request to the overloaded node
            MPI_Send(&req, 1, MPI_INT, overloaded_node, TAG_STEAL_REQ, MPI_COMM_WORLD);
            
            MPI_Probe(overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, &status);
            int bytes; MPI_Get_count(&status, MPI_BYTE, &bytes);
            
            Trade* stolen = (Trade*)malloc(bytes);
            if (bytes > 0) MPI_Recv(stolen, bytes, MPI_BYTE, overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            else MPI_Recv(NULL, 0, MPI_BYTE, overloaded_node, TAG_STOLEN_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            int stolen_count = bytes / sizeof(Trade);
            printf("[Master] Acquired %d tasks from Node %d. Forwarding to Node %d...\n", stolen_count, overloaded_node, starving_node);
            
            if (stolen_count > 0) {
                MPI_Send(stolen, bytes, MPI_BYTE, starving_node, TAG_WORK, MPI_COMM_WORLD);
                all_idle_count = 0; // Work was redistributed, reset
            } else {
                all_idle_count++;
                if (all_idle_count >= NUM_WORKERS) {
                    if(stolen) free(stolen);
                    printf("\n[Master] All workers have finished processing. Ending early.\n");
                    break;
                }
            }
            if(stolen) free(stolen);
        }
        usleep(PROBE_INTERVAL_USEC); 
    }

    printf("\n[Master] SIMULATION COMPLETE. Sending shutdown signals.\n");
    int kill = 1;
    MPI_Send(&kill, 1, MPI_INT, 1, TAG_KILL_SIGNAL, MPI_COMM_WORLD);
    MPI_Send(&kill, 1, MPI_INT, 2, TAG_KILL_SIGNAL, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double total_time = end_time - start_time;

    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║   RESULTS: HPC LOAD BALANCED                               ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║                                                            ║\n");
    printf("║  Total Execution Time:  %8.3f seconds                   ║\n", total_time);
    printf("║  Total Tasks Assigned:  %d                            ║\n", INITIAL_TASKS_NODE_1 + INITIAL_TASKS_NODE_2);
    printf("║                                                            ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    free(batch);
}