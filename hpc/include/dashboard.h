#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <stdio.h>
#include <string.h>

/**
 * Dashboard Integration for HPC Load Balancer
 * 
 * Optional module to send status updates to the Flask dashboard via HTTP
 *
 * Usage:
 *   dashboard_init("http://127.0.0.1:5000");
 *   dashboard_update_worker(rank, queue_size, tasks_processed, "ACTIVE");
 *   dashboard_report_steal_event(source, dest, tasks_stolen);
 *   dashboard_cleanup();
 */

// Initialize dashboard connection
void dashboard_init(const char* server_url);

// Update worker status
void dashboard_update_worker(int rank, int queue_size, int tasks_processed, const char* status);

// Report steal event
void dashboard_report_steal_event(int source_rank, int dest_rank, int tasks_stolen);

// Update overall status
void dashboard_set_status(const char* status);

// Cleanup
void dashboard_cleanup();

// Check if dashboard is enabled
int dashboard_is_enabled();

#endif
