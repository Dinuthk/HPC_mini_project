#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// CONFIGURATION: Distributed Load Balancer System
// ============================================================================

// Total number of processes (1 Master + N Workers)
// Must be at least 3 (1 Master + 2 Workers)
#define WORLD_SIZE 3

// Number of Workers (Total Processes - 1 Master)
#define NUM_WORKERS (WORLD_SIZE - 1)

// Master process rank
#define MASTER_RANK 0

// ============================================================================
// Task Configuration
// ============================================================================
#define INITIAL_TASKS_NODE_1 40000
#define INITIAL_TASKS_NODE_2 6000

#define SIMULATION_DURATION_SECONDS 2000.0
#define PROBE_INTERVAL_USEC 20000

// ============================================================================
// Display System Configuration
// ============================================================================
void display_system_config(int world_rank, int world_size);

#endif
