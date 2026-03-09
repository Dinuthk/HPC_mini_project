#include "common.h"
#include "master.h"
#include "worker.h"

// Define Global Variables exactly once
TradeQueue task_queue;
pthread_mutex_t queue_mutex;
int simulation_running = 1;
int waiting_for_work = 0;
SimConfig config;
WorkerMetrics worker_metrics;

// Queue Implementations
void init_queue(TradeQueue* q) { q->front = 0; q->rear = -1; q->count = 0; }

void push_queue(TradeQueue* q, Trade t) {
    if (q->count < MAX_QUEUE_SIZE) {
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
        q->data[q->rear] = t;
        q->count++;
    }
}

Trade pop_front_queue(TradeQueue* q) {
    Trade t = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    q->count--;
    return t;
}

Trade pop_rear_queue(TradeQueue* q) {
    Trade t = q->data[q->rear];
    q->rear = (q->rear - 1 + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
    q->count--;
    return t;
}

// Utility Functions
void parse_config(int argc, char** argv, SimConfig* cfg) {
    // Set defaults
    cfg->total_tasks = DEFAULT_TOTAL_TASKS;
    cfg->imbalance_ratio = DEFAULT_IMBALANCE_RATIO;
    cfg->batch_size = DEFAULT_BATCH_SIZE;
    cfg->low_watermark = DEFAULT_LOW_WATERMARK;
    cfg->simulation_time = 10.0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            cfg->total_tasks = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            cfg->imbalance_ratio = atof(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0 && i + 1 < argc) {
            cfg->batch_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-w") == 0 && i + 1 < argc) {
            cfg->low_watermark = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            cfg->simulation_time = atof(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            MPI_Finalize();
            exit(0);
        }
    }
}

void print_usage(const char* program_name) {
    printf("Usage: mpirun -np <N> %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -t <tasks>      Total tasks (default: %d)\n", DEFAULT_TOTAL_TASKS);
    printf("  -r <ratio>      Imbalance ratio (default: %.1f)\n", DEFAULT_IMBALANCE_RATIO);
    printf("  -b <batch>      Batch size (default: %d)\n", DEFAULT_BATCH_SIZE);
    printf("  -w <watermark>  Low watermark (default: %d)\n", DEFAULT_LOW_WATERMARK);
    printf("  -s <seconds>    Simulation time (default: 10.0)\n");
    printf("  -h, --help      Show this help message\n");
}

double get_wall_time() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (double)time.tv_sec + (double)time.tv_usec * 1e-6;
}

void export_metrics_to_csv(const char* filename, WorkerMetrics* metrics, int num_workers, double total_time) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("[Error] Could not open %s for writing\n", filename);
        return;
    }
    
    fprintf(fp, "worker_id,tasks_processed,steal_attempts,steal_successes,total_time,compute_time,idle_time,throughput\n");
    
    for (int i = 0; i < num_workers; i++) {
        double throughput = metrics[i].total_time > 0 ? metrics[i].tasks_processed / metrics[i].total_time : 0.0;
        fprintf(fp, "%d,%d,%d,%d,%.6f,%.6f,%.6f,%.2f\n",
                metrics[i].worker_id,
                metrics[i].tasks_processed,
                metrics[i].steal_attempts,
                metrics[i].steal_successes,
                metrics[i].total_time,
                metrics[i].compute_time,
                metrics[i].idle_time,
                throughput);
    }
    
    fprintf(fp, "\n# Total simulation time: %.6f seconds\n", total_time);
    fprintf(fp, "# Total tasks processed: %d\n", 
            metrics[0].tasks_processed + (num_workers > 1 ? metrics[1].tasks_processed : 0));
    
    fclose(fp);
    printf("[Master] Metrics exported to %s\n", filename);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size < 3) {
        if (world_rank == 0) printf("[Error] Need at least 3 nodes.\n");
        MPI_Finalize();
        return 1;
    }

    // Parse configuration (all ranks parse to avoid broadcast complexity)
    parse_config(argc, argv, &config);

    if (world_rank == 0) {
        printf("====================================================\n");
        printf("Configuration:\n");
        printf("  Total tasks: %d\n", config.total_tasks);
        printf("  Imbalance ratio: %.1f\n", config.imbalance_ratio);
        printf("  Batch size: %d\n", config.batch_size);
        printf("  Low watermark: %d\n", config.low_watermark);
        printf("  Simulation time: %.1f seconds\n", config.simulation_time);
        printf("====================================================\n");
        run_master(world_rank, world_size);
    } else {
        run_worker(world_rank, world_size);
    }

    MPI_Finalize();
    return 0;
}