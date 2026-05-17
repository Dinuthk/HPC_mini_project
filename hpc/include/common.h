#ifndef COMMON_H
#define COMMON_H

#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Communication Tags
#define TAG_WORK 1
#define TAG_KILL_SIGNAL 2
#define TAG_IDLE 3          
#define TAG_STEAL_REQ 4     
#define TAG_STOLEN_WORK 5   

#define MAX_QUEUE_SIZE 1000000
#define BATCH_SIZE 1024 
#define LOW_WATERMARK 1000

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

// Queue Function Prototypes
void init_queue(TradeQueue* q);
void push_queue(TradeQueue* q, Trade t);
Trade pop_front_queue(TradeQueue* q);
Trade pop_rear_queue(TradeQueue* q);

#endif