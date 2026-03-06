#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

// 1. Create the Trade struct using a typedef for pure C
typedef struct {
    int stock_id;
    double price;
    double volume;
    long timestamp;
    double complexity_weight;
} Trade;

int main(int argc, char** argv) {
    // 2. Initialize MS-MPI
    MPI_Init(&argc, &argv);

    int world_rank;
    int world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Enforce cluster size
    if (world_size < 3) {
        if (world_rank == 0) {
            printf("[Error] This simulation requires at least 3 processes (1 Master, 2 Workers).\n");
        }
        MPI_Finalize();
        return 1;
    }

    const int TOTAL_TRADES = 100000;
    const int TRADES_PER_WORKER = 50000;

    // ---------------- MASTER NODE (Node 0) ----------------
    if (world_rank == 0) {
        printf("[Master] Generating %d dummy trades...\n", TOTAL_TRADES);
        
        // Dynamically allocate memory for 100,000 trades using malloc
        Trade* all_trades = (Trade*)malloc(TOTAL_TRADES * sizeof(Trade));
        if (all_trades == NULL) {
            printf("Memory allocation failed!\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // Populate dummy data
        for (int i = 0; i < TOTAL_TRADES; i++) {
            all_trades[i].stock_id = i % 500;
            all_trades[i].price = 150.0 + (i % 10);
            all_trades[i].volume = 1000.0;
            all_trades[i].timestamp = 1700000000 + i;
            all_trades[i].complexity_weight = 1.0;
        }

        printf("[Master] Pushing 50,000 trades to Node 1 and Node 2...\n");

        // 3. Send trades to Node 1
        MPI_Send(all_trades, TRADES_PER_WORKER * sizeof(Trade), MPI_BYTE, 1, 0, MPI_COMM_WORLD);
        
        // Send trades to Node 2 (Offset the pointer by adding TRADES_PER_WORKER)
        MPI_Send(all_trades + TRADES_PER_WORKER, TRADES_PER_WORKER * sizeof(Trade), MPI_BYTE, 2, 0, MPI_COMM_WORLD);

        // 4. Wait for success messages from Workers
        int ack1, ack2;
        MPI_Recv(&ack1, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("[Master] Received Success ACK (%d) from Worker 1.\n", ack1);
        
        MPI_Recv(&ack2, 1, MPI_INT, 2, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("[Master] Received Success ACK (%d) from Worker 2.\n", ack2);

        // Free the allocated memory
        free(all_trades);
    } 
    
    // ---------------- WORKER NODES (Nodes 1 & 2) ----------------
    else if (world_rank == 1 || world_rank == 2) {
        // Allocate memory to receive the incoming trades
        Trade* my_trades = (Trade*)malloc(TRADES_PER_WORKER * sizeof(Trade));
        if (my_trades == NULL) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Receive trades
        MPI_Recv(my_trades, TRADES_PER_WORKER * sizeof(Trade), MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        printf("[Worker %d] Received 50000 trades. (Validating first stock_id: %d)\n", world_rank, my_trades[0].stock_id);

        // Send a success message back to Master
        int success_msg = 1; 
        MPI_Send(&success_msg, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);

        // Free the memory
        free(my_trades);
    }

    // Clean up
    MPI_Finalize();
    return 0;
}