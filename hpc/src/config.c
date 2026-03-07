#include "common.h"
#include "config.h"

void display_system_config(int world_rank, int world_size) {
    if (world_rank == 0) {
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║     DISTRIBUTED LOAD BALANCER SYSTEM CONFIGURATION         ║\n");
        printf("╠════════════════════════════════════════════════════════════╣\n");
        printf("║                                                            ║\n");
        printf("║  Status:           INITIALIZING                            ║\n");
        printf("║  Total World Size: %d                                      ║\n", world_size);
        printf("║  Master Node:      Rank 0                                  ║\n");
        printf("║  Number of Workers: %d                                     ║\n", NUM_WORKERS);
        printf("║                                                            ║\n");
        printf("║  Worker Nodes:                                             ║\n");
        
        for (int i = 1; i < world_size; i++) {
            printf("║    - Worker %d (Rank %d)                                    ║\n", i, i);
        }
        
        printf("║                                                            ║\n");
        printf("║  Simulation Parameters:                                    ║\n");
        printf("║    - Duration: %.1f seconds                               ║\n", SIMULATION_DURATION_SECONDS);
        printf("║    - Initial Load Node 1: %d tasks                         ║\n", INITIAL_TASKS_NODE_1);
        printf("║    - Initial Load Node 2: %d tasks                         ║\n", INITIAL_TASKS_NODE_2);
        printf("║                                                            ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
    }
    
    // Barrier to ensure all nodes are synchronized before starting
    MPI_Barrier(MPI_COMM_WORLD);
}
