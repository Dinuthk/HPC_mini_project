#ifndef COMMON_H
#define COMMON_H

#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

// Communication Tags
#define TAG_WORK 1
#define TAG_KILL_SIGNAL 2
#define TAG_IDLE 3          
#define TAG_STEAL_REQ 4     
#define TAG_STOLEN_WORK 5
#define TAG_WORKER_DONE 6
#define TAG_METRICS 7   

#define MAX_QUEUE_SIZE 200000
#define DEFAULT_BATCH_SIZE 1024 
#define DEFAULT_LOW_WATERMARK 1000
#define DEFAULT_TOTAL_TASKS 90000
#define DEFAULT_IMBALANCE_RATIO 8.0

// Configuration Struct
typedef struct {
    int total_tasks;           // Total number of tasks
    double imbalance_ratio;    // Ratio of tasks between workers (e.g., 8.0 means 8:1)
    int batch_size;            // Processing batch size
    int low_watermark;         // Threshold to request more work
    double simulation_time;    // Max simulation time in seconds
} SimConfig;

// Worker Metrics Struct
typedef struct {
    int worker_id;
    int tasks_processed;
    int steal_attempts;
    int steal_successes;
    double total_time;
    double compute_time;
    double idle_time;
} WorkerMetrics;

// Structs
typedef struct {
    int stock_id;
    double price;
    double volume;
    long timestamp;
    double complexity_weight;
} Trade;

typedef struct {
    Trade data[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
} TradeQueue;

// Global Variables (Declared here, defined in main.c)
extern TradeQueue task_queue;
extern pthread_mutex_t queue_mutex;
extern int simulation_running;
extern int waiting_for_work;
extern SimConfig config;
extern WorkerMetrics worker_metrics;

// Queue Function Prototypes
void init_queue(TradeQueue* q);
void push_queue(TradeQueue* q, Trade t);
Trade pop_front_queue(TradeQueue* q);
Trade pop_rear_queue(TradeQueue* q);

// Utility Functions
void parse_config(int argc, char** argv, SimConfig* cfg);
void print_usage(const char* program_name);
void export_metrics_to_csv(const char* filename, WorkerMetrics* metrics, int num_workers, double total_time);
double get_wall_time();

#endif