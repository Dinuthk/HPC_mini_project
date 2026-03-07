#include "dashboard.h"
#include <stdlib.h>

static char dashboard_url[256] = {0};
static int dashboard_enabled = 0;

void dashboard_init(const char* server_url) {
    if (server_url && strlen(server_url) < sizeof(dashboard_url) - 1) {
        strcpy(dashboard_url, server_url);
        dashboard_enabled = 1;
        printf("[Dashboard] Connected to: %s\n", dashboard_url);
    } else {
        printf("[Dashboard] Failed to initialize - invalid URL\n");
    }
}

void dashboard_update_worker(int rank, int queue_size, int tasks_processed, const char* status) {
    if (!dashboard_enabled) return;
    
    // Build curl command to update worker status
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "curl -s -X POST %s/api/status/update "
        "-H 'Content-Type: application/json' "
        "-d '{\"worker_update\": {\"rank\": %d, \"queue_size\": %d, \"tasks_processed\": %d, \"status\": \"%s\"}}' "
        "> /dev/null 2>&1 &",
        dashboard_url, rank, queue_size, tasks_processed, status);
    
    system(cmd);
}

void dashboard_report_steal_event(int source_rank, int dest_rank, int tasks_stolen) {
    if (!dashboard_enabled) return;
    
    // Build curl command to report steal event
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "curl -s -X POST %s/api/status/update "
        "-H 'Content-Type: application/json' "
        "-d '{\"steal_event\": {\"source_rank\": %d, \"dest_rank\": %d, \"tasks_stolen\": %d}}' "
        "> /dev/null 2>&1 &",
        dashboard_url, source_rank, dest_rank, tasks_stolen);
    
    system(cmd);
}

void dashboard_set_status(const char* status) {
    if (!dashboard_enabled) return;
    
    // Build curl command to update overall status
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "curl -s -X POST %s/api/status/update "
        "-H 'Content-Type: application/json' "
        "-d '{\"status\": \"%s\"}' "
        "> /dev/null 2>&1 &",
        dashboard_url, status);
    
    system(cmd);
}

void dashboard_cleanup(void) {
    if (dashboard_enabled) {
        printf("[Dashboard] Disconnected\n");
        dashboard_enabled = 0;
    }
}

int dashboard_is_enabled(void) {
    return dashboard_enabled;
}
