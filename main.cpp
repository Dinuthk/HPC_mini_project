#include <mpi.h>
#include <omp.h>
#include <iostream>

int main(int argc, char** argv) {
    // 1. Start the Network Layer
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank); // 0=Master, 1=Worker1, 2=Worker2
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the name of the laptop running this process
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    std::cout << "Hello from Node " << world_rank << " running on laptop: " << processor_name << std::endl;

    // 2. Start the Parallel Layer (Wait for everyone to catch up first)
    MPI_Barrier(MPI_COMM_WORLD); 

    // OpenMP will now spin up threads on each laptop's CPU
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int total_threads = omp_get_num_threads();
        
        // Print safely so threads don't write over each other
        #pragma omp critical
        {
            std::cout << "[Node " << world_rank << "] Thread " << thread_id 
                      << " out of " << total_threads << " reporting for duty!" << std::endl;
        }
    }

    // 3. Shut down cleanly
    MPI_Finalize();
    return 0;
}