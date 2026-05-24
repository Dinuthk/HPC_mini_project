#include <cuda_runtime.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================
   CONFIG  (mirrors config.h / common.h from the CPU version)
   ============================================================ */
#define INITIAL_TASKS_NODE_1   49998
#define INITIAL_TASKS_NODE_2   10000
#define TOTAL_TASKS            (INITIAL_TASKS_NODE_1 + INITIAL_TASKS_NODE_2)
#define SIMULATIONS_PER_TRADE  50000
#define BATCH_SIZE             1024
#define LOW_WATERMARK          1000
#define NUM_STREAMS            2
#define MAX_STEAL_ROUNDS       20

/* ============================================================
   DATA STRUCTURES  (mirrors Trade / TradeQueue from common.h)
   ============================================================ */
typedef struct {
    int stock_id;
    double price;
    double volume;
    long timestamp;
    double complexity_weight;
} Trade;

typedef struct {
    Trade *data;
    int front;
    int rear;
    int count;
    int capacity;
} TradeQueue;

/* ============================================================
   HOST QUEUE HELPERS  (mirrors queue ops in main.c)
   ============================================================ */
void init_queue(TradeQueue *q, int cap) {
    cudaHostAlloc((void**)&q->data, cap * sizeof(Trade), cudaHostAllocDefault);
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    q->capacity = cap;
}

void free_queue(TradeQueue *q) {
    cudaFreeHost(q->data);
}

int push_queue(TradeQueue *q, Trade t) {
    if (q->count >= q->capacity) return 0;
    q->data[q->rear] = t;
    q->rear = (q->rear + 1) % q->capacity;
    q->count++;
    return 1;
}

int pop_front(TradeQueue *q, Trade *batch, int n) {
    int got = 0;
    while (got < n && q->count > 0) {
        batch[got++] = q->data[q->front];
        q->front = (q->front + 1) % q->capacity;
        q->count--;
    }
    return got;
}

int pop_rear(TradeQueue *q, Trade *batch, int n) {
    int got = 0;
    while (got < n && q->count > 0) {
        q->rear = (q->rear - 1 + q->capacity) % q->capacity;
        batch[got++] = q->data[q->rear];
        q->count--;
    }
    return got;
}

/* ============================================================
   CUDA KERNEL  (GPU equivalent of compute_risk_batch + OpenMP)
   ============================================================ */
__global__ void compute_risk_kernel(
    const Trade * __restrict__ trades,
    double * __restrict__ results,
    int n_trades,
    int simulations)
{
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n_trades) return;

    double base_price = trades[tid].price;
    double volume = trades[tid].volume;
    double weight = trades[tid].complexity_weight;
    double total = 0.0;

    for (int i = 0; i < simulations; i++) {
        double pseudo_rand = (double)(i % 100) / 100.0;
        double drift = (base_price * 0.02) - (volume * 0.00005);
        double volatility = sin(base_price * pseudo_rand) * cos(volume * 0.001) * weight;
        double simulated_price = base_price * exp(drift + volatility + log(1.0 + pseudo_rand));
        total += simulated_price;
    }
    results[tid] = total;
}

/* ============================================================
   DEVICE BUFFERS  - one set per stream
   ============================================================ */
typedef struct {
    Trade *d_trades;
    double *d_results;
    double *h_results;
    cudaStream_t stream;
    int stream_id;
    long trades_processed;
    double total_risk;
} StreamCtx;

void init_stream_ctx(StreamCtx *ctx, int id) {
    ctx->stream_id = id;
    ctx->trades_processed = 0;
    ctx->total_risk = 0.0;
    cudaStreamCreate(&ctx->stream);
    cudaMalloc((void**)&ctx->d_trades, BATCH_SIZE * sizeof(Trade));
    cudaMalloc((void**)&ctx->d_results, BATCH_SIZE * sizeof(double));
    cudaHostAlloc((void**)&ctx->h_results, BATCH_SIZE * sizeof(double), cudaHostAllocDefault);
}

void free_stream_ctx(StreamCtx *ctx) {
    cudaStreamDestroy(ctx->stream);
    cudaFree(ctx->d_trades);
    cudaFree(ctx->d_results);
    cudaFreeHost(ctx->h_results);
}

void launch_batch(StreamCtx *ctx, Trade *batch, int n) {
    cudaMemcpyAsync(ctx->d_trades, batch, n * sizeof(Trade), cudaMemcpyHostToDevice, ctx->stream);

    int threads = 256;
    int blocks = (n + threads - 1) / threads;
    compute_risk_kernel<<<blocks, threads, 0, ctx->stream>>>(
        ctx->d_trades, ctx->d_results, n, SIMULATIONS_PER_TRADE);

    cudaMemcpyAsync(ctx->h_results, ctx->d_results, n * sizeof(double), cudaMemcpyDeviceToHost, ctx->stream);
}

void collect_results(StreamCtx *ctx, int n) {
    cudaStreamSynchronize(ctx->stream);
    for (int i = 0; i < n; i++) {
        ctx->total_risk += ctx->h_results[i];
    }
    ctx->trades_processed += n;
}

/* ============================================================
   WORK-STEALING PROTOCOL
   ============================================================ */
int steal_work(TradeQueue *victim, TradeQueue *beneficiary, int stream_id) {
    if (victim->count < 2) return 0;

    int steal_count = victim->count / 2;
    Trade *stolen = (Trade*)malloc(steal_count * sizeof(Trade));
    int got = pop_rear(victim, stolen, steal_count);

    printf("[Master] Stream %d is idle (<%d tasks). Stealing %d tasks from Stream %d -> redistributing.\n",
           stream_id, LOW_WATERMARK, got, 1 - stream_id);

    for (int i = 0; i < got; i++) {
        push_queue(beneficiary, stolen[i]);
    }

    free(stolen);
    return got;
}

/* ============================================================
   MASTER SCHEDULING LOOP
   ============================================================ */
void run_load_balancer(TradeQueue *queues, StreamCtx *ctxs) {
    Trade batch[BATCH_SIZE];
    int active[NUM_STREAMS] = {1, 1};
    int steal_rounds = 0;

    printf("\n[Master] Starting load-balanced GPU simulation...\n");
    printf("[Master] Stream 0 initial tasks : %d\n", queues[0].count);
    printf("[Master] Stream 1 initial tasks : %d\n", queues[1].count);
    printf("\n");

    while (active[0] || active[1]) {
        for (int s = 0; s < NUM_STREAMS; s++) {
            if (!active[s]) continue;

            if (queues[s].count <= LOW_WATERMARK && steal_rounds < MAX_STEAL_ROUNDS) {
                int victim = 1 - s;
                if (queues[victim].count > 0) {
                    steal_work(&queues[victim], &queues[s], s);
                    steal_rounds++;
                }
            }

            int n = pop_front(&queues[s], batch, BATCH_SIZE);
            if (n > 0) {
                launch_batch(&ctxs[s], batch, n);
                collect_results(&ctxs[s], n);

                if (ctxs[s].trades_processed % 5000 == 0 || queues[s].count == 0) {
                    printf("[Worker %d] Processed %ld trades | Remaining: %d | Risk: %.2e\n",
                           s, ctxs[s].trades_processed, queues[s].count, ctxs[s].total_risk);
                }
            } else {
                active[s] = 0;
                printf("[Worker %d] Queue exhausted. Shutting down stream.\n", s);
            }
        }
    }
}

/* ============================================================
   MAIN
   ============================================================ */
int main(void) {
    struct timespec t0, t1;

    int dev = 0;
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, dev);
    printf("=================================================\n");
    printf(" CUDA Load Balancer - GPU Platform\n");
    printf("=================================================\n");
    printf(" Device      : %s\n", prop.name);
    printf(" SMs         : %d\n", prop.multiProcessorCount);
    printf(" Global Mem  : %.1f GB\n", (double)prop.totalGlobalMem / (1024*1024*1024));
    printf(" Streams     : %d  (= MPI workers)\n", NUM_STREAMS);
    printf(" Batch size  : %d trades\n", BATCH_SIZE);
    printf(" Simulations : %d per trade\n", SIMULATIONS_PER_TRADE);
    printf("=================================================\n\n");

    TradeQueue queues[NUM_STREAMS];
    init_queue(&queues[0], TOTAL_TASKS + 10);
    init_queue(&queues[1], TOTAL_TASKS + 10);

    srand(42);
    for (int i = 0; i < INITIAL_TASKS_NODE_1; i++) {
        Trade t = { i, 150.0 + (i % 50), 100.0 + (i % 20), (long)i, 1.0 + (double)(i % 5) * 0.1 };
        push_queue(&queues[0], t);
    }
    for (int i = 0; i < INITIAL_TASKS_NODE_2; i++) {
        Trade t = { INITIAL_TASKS_NODE_1 + i, 200.0 + (i % 30), 80.0 + (i % 10),
                    (long)(INITIAL_TASKS_NODE_1 + i), 0.8 + (double)(i % 3) * 0.1 };
        push_queue(&queues[1], t);
    }

    StreamCtx ctxs[NUM_STREAMS];
    for (int s = 0; s < NUM_STREAMS; s++) {
        init_stream_ctx(&ctxs[s], s);
    }

    clock_gettime(CLOCK_MONOTONIC, &t0);
    run_load_balancer(queues, ctxs);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    printf("\n=================================================\n");
    printf(" RESULT SUMMARY\n");
    printf("=================================================\n");
    long total_trades = 0;
    double total_risk = 0.0;
    for (int s = 0; s < NUM_STREAMS; s++) {
        printf(" Stream %d : %ld trades | Risk = %.6e\n", s, ctxs[s].trades_processed, ctxs[s].total_risk);
        total_trades += ctxs[s].trades_processed;
        total_risk += ctxs[s].total_risk;
    }
    printf("-------------------------------------------------\n");
    printf(" Total trades    : %ld\n", total_trades);
    printf(" Total risk      : %.6e\n", total_risk);
    printf(" Wall-clock time : %.4f s\n", elapsed);
    printf(" Throughput      : %.1f trades/s\n", total_trades / elapsed);
    printf("=================================================\n");

    for (int s = 0; s < NUM_STREAMS; s++) {
        free_stream_ctx(&ctxs[s]);
        free_queue(&queues[s]);
    }
    return 0;
}
