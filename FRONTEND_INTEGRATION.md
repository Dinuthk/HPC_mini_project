# Frontend Dashboard Integration Guide

## Overview

This guide explains how to integrate the web-based frontend dashboard with your HPC load balancer MPI application.

## System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                  Web Browser                             │
│              (User Interface)                            │
└─────────────────┬───────────────────────────────────────┘
                  │
                  │ HTTP/REST API
                  │
         ┌────────▼──────────┐
         │  Flask Dashboard  │
         │  (app.py)         │
         └────────▲──────────┘
                  │
                  │ Status Updates
                  │
    ┌─────────────┴──────────────┐
    │                            │
┌───▼─────────────┐    ┌────────▼──────────┐
│   Master Node   │    │  Worker Nodes     │
│  (Rank 0)       │    │  (Rank 1, 2, ...) │
│                 │    │                   │
│ MPI Application │    │ MPI Application   │
│ (C Program)     │    │ (C Program)       │
└─────────────────┘    └───────────────────┘
```

## Quick Start

### 1. Start the Dashboard

In the `frontend/` directory:

```bash
# Install dependencies (first time only)
pip install -r requirements.txt

# Start the Flask server
python app.py
```

The dashboard will be available at: **http://127.0.0.1:5000**

### 2. Connect from MPI Application

The MPI application can optionally send status updates via HTTP curl commands.

#### Method A: Using the Dashboard Module (Recommended)

1. **Add headers to your MPI code:**
```c
#include "dashboard.h"
```

2. **Initialize in main():**
```c
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    // Initialize dashboard connection
    dashboard_init("http://127.0.0.1:5000");
    
    // ... rest of your MPI code
}
```

3. **Send updates from your worker/master:**
```c
// From master - update status
dashboard_set_status("RUNNING");

// From worker - update stats
dashboard_update_worker(rank, queue_size, tasks_processed, "ACTIVE");

// Report steal events
dashboard_report_steal_event(source_rank, dest_rank, tasks_stolen);
```

4. **Cleanup on exit:**
```c
MPI_Finalize();
dashboard_cleanup();
```

#### Method B: Manual curl Commands

Alternatively, send HTTP requests directly using curl:

```bash
# Update worker status
curl -X POST http://127.0.0.1:5000/api/status/update \
  -H "Content-Type: application/json" \
  -d '{
    "worker_update": {
      "rank": 1,
      "queue_size": 5000,
      "tasks_processed": 10000,
      "status": "ACTIVE"
    }
  }'

# Report steal event
curl -X POST http://127.0.0.1:5000/api/status/update \
  -H "Content-Type: application/json" \
  -d '{
    "steal_event": {
      "source_rank": 2,
      "dest_rank": 1,
      "tasks_stolen": 5000
    }
  }'

# Update overall status
curl -X POST http://127.0.0.1:5000/api/status/update \
  -H "Content-Type: application/json" \
  -d '{"status": "RUNNING"}'
```

## Integration Points

### Master Node Integration

In `master.c`, add updates at key points:

```c
#include "dashboard.h"

void run_master(int world_rank, int world_size) {
    // Initialize
    dashboard_set_status("RUNNING");
    
    // When monitoring
    printf("[Master] ALERT! Node %d is starving!\n", starving_node);
    dashboard_report_steal_event(overloaded_node, starving_node, stolen_count);
    
    // When complete
    dashboard_set_status("COMPLETE");
}
```

### Worker Node Integration

In `worker.c`, add updates periodically:

```c
#include "dashboard.h"

void run_worker(int world_rank, int world_size) {
    int queue_size = 0;
    int tasks_processed = 0;
    
    // Periodically report status (every N iterations)
    if (iteration % 100 == 0) {
        dashboard_update_worker(world_rank, queue_size, tasks_processed, "ACTIVE");
    }
}
```

## API Reference

### POST /api/status/update

Update system status with worker information or events.

**Request Body (Worker Update):**
```json
{
  "worker_update": {
    "rank": 1,
    "queue_size": 5000,
    "tasks_processed": 10000,
    "status": "ACTIVE"
  }
}
```

**Request Body (Steal Event):**
```json
{
  "steal_event": {
    "source_rank": 2,
    "dest_rank": 1,
    "tasks_stolen": 5000
  }
}
```

**Request Body (Status Update):**
```json
{
  "status": "RUNNING"
}
```

### GET /api/status

Get complete system status.

**Response:**
```json
{
  "world_size": 3,
  "num_workers": 2,
  "master_rank": 0,
  "status": "RUNNING",
  "workers": [
    {
      "rank": 1,
      "status": "ACTIVE",
      "queue_size": 5000,
      "tasks_processed": 10000,
      "last_update": "2026-03-07T10:30:45.123456"
    },
    {
      "rank": 2,
      "status": "IDLE",
      "queue_size": 0,
      "tasks_processed": 50000,
      "last_update": "2026-03-07T10:30:44.987654"
    }
  ],
  "steal_events": [
    {
      "timestamp": "2026-03-07T10:30:40.123456",
      "source_rank": 1,
      "dest_rank": 2,
      "tasks_stolen": 3000
    }
  ]
}
```

### GET /api/config

Get system configuration.

**Response:**
```json
{
  "world_size": 3,
  "num_workers": 2,
  "initial_tasks_node_1": 80000,
  "initial_tasks_node_2": 10000,
  "simulation_duration": 6.0,
  "probe_interval_ms": 20
}
```

### POST /api/config

Update system configuration.

**Request Body:**
```json
{
  "initial_tasks_node_1": 100000,
  "initial_tasks_node_2": 20000,
  "simulation_duration": 10.0
}
```

### POST /api/reset

Reset system statistics.

### GET /api/stats/export

Export current stats as JSON file.

**Response:**
```json
{
  "message": "Stats exported",
  "filename": "stats_20260307_103045.json"
}
```

## Network Configuration

### Default Configuration
- **Host**: 127.0.0.1 (localhost)
- **Port**: 5000
- **URL**: http://127.0.0.1:5000

### Remote Access

To access the dashboard from other machines:

1. **Edit `app.py`:**
```python
app.run(debug=True, host='0.0.0.0', port=5000)
```

2. **Access from other machine:**
```
http://<your-machine-ip>:5000
```

### Docker Deployment (Optional)

Create a `Dockerfile`:

```dockerfile
FROM python:3.11-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install -r requirements.txt

COPY . .

EXPOSE 5000

CMD ["python", "app.py"]
```

Build and run:
```bash
docker build -t hpc-dashboard .
docker run -p 5000:5000 hpc-dashboard
```

## Performance Considerations

### Dashboard Module Performance
- HTTP requests sent asynchronously (non-blocking)
- Updates happen in background via `curl` with `&` (fork)
- Minimal impact on MPI application performance

### Polling Interval
- Dashboard updates every 2 seconds by default
- Adjust in `static/js/main.js` for higher/lower frequency
- Balance between responsiveness and server load

### Data Retention
- Keeps last 100 steal events in memory
- Exported stats are saved to disk
- Configure database backend for large-scale deployments

## Security Notes

### Current Implementation
- No authentication (suitable for local/internal networks)
- HTTP (not HTTPS)
- CORS not restricted

### Production Hardening
1. Add CORS restrictions:
```python
from flask_cors import CORS
CORS(app, resources={r"/api/*": {"origins": ["127.0.0.1"]}})
```

2. Add authentication:
```python
from flask_httpauth import HTTPBasicAuth
auth = HTTPBasicAuth()
```

3. Use HTTPS:
```bash
pip install cert=adhoc
```

## Troubleshooting

### "Connection refused" error
**Problem**: MPI app can't connect to dashboard
**Solution**: 
- Ensure Flask server is running: `python app.py`
- Check firewall settings
- Verify URL in `dashboard_init()` is correct

### Dashboard shows stale data
**Problem**: Status not updating
**Solution**:
- Check MPI app has curl installed
- Verify network connectivity: `curl http://127.0.0.1:5000/health`
- Check Flask server logs for errors

### Charts not updating
**Problem**: Queue and task charts frozen
**Solution**:
- Ensure worker updates are being sent
- Check browser console for JavaScript errors
- Try refreshing the page

### Port 5000 already in use
**Problem**: "Address already in use"
**Solution**:
```bash
# Find process using port 5000
lsof -i :5000

# Or change port in app.py
app.run(port=5001)
```

## Examples

### Complete Integration Example

```c
#include "common.h"
#include "config.h"
#include "dashboard.h"
#include "master.h"
#include "worker.h"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    // Initialize dashboard
    dashboard_init("http://127.0.0.1:5000");
    dashboard_set_status("INITIALIZING");
    
    display_system_config(world_rank, world_size);
    
    if (world_rank == 0) {
        run_master(world_rank, world_size);
    } else {
        run_worker(world_rank, world_size);
    }
    
    dashboard_cleanup();
    MPI_Finalize();
    return 0;
}
```

## Next Steps

1. **Add dashboard module to Makefile:**
```makefile
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Now includes dashboard.c automatically
```

2. **Rebuild the project:**
```bash
cd hpc
make clean && make
```

3. **Run with dashboard:**
```bash
# Terminal 1: Start dashboard
cd frontend
python app.py

# Terminal 2: Run MPI app
cd ../hpc
mpirun -np 3 ./bin/load_balancer
```

4. **View in browser:**
```
http://127.0.0.1:5000
```

## Support

For issues, check:
- Flask dashboard logs in terminal
- Browser console (F12 > Console)
- MPI application stderr output
- Network connectivity with `curl http://127.0.0.1:5000/health`
