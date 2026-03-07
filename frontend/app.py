#!/usr/bin/env python3
"""
HPC Load Balancer Dashboard
Web-based monitoring and control interface for distributed load balancer
"""

from flask import Flask, render_template, jsonify, request
import json
import os
from datetime import datetime
from pathlib import Path

app = Flask(__name__)
app.config['JSON_SORT_KEYS'] = False

# Configuration
STATS_DIR = Path(__file__).parent / "stats"
STATS_DIR.mkdir(exist_ok=True)

# In-memory stats (in production, use database)
system_stats = {
    "world_size": 3,
    "num_workers": 2,
    "master_rank": 0,
    "status": "INITIALIZING",
    "start_time": None,
    "workers": [
        {"rank": 1, "status": "IDLE", "queue_size": 0, "tasks_processed": 0, "last_update": None},
        {"rank": 2, "status": "IDLE", "queue_size": 0, "tasks_processed": 0, "last_update": None}
    ],
    "steal_events": [],
    "simulation_duration": 6.0,
    "simulation_elapsed": 0.0
}

config_data = {
    "world_size": 3,
    "num_workers": 2,
    "initial_tasks_node_1": 80000,
    "initial_tasks_node_2": 10000,
    "simulation_duration": 6.0,
    "probe_interval_ms": 20
}


# ============================================================================
# API ENDPOINTS
# ============================================================================

@app.route('/')
def index():
    """Serve the dashboard homepage"""
    return render_template('index.html')


@app.route('/api/config', methods=['GET'])
def get_config():
    """Get system configuration"""
    return jsonify(config_data)


@app.route('/api/config', methods=['POST'])
def update_config():
    """Update system configuration"""
    global config_data
    data = request.get_json()
    
    # Validate inputs
    if 'world_size' in data and data['world_size'] < 3:
        return jsonify({"error": "World size must be at least 3"}), 400
    
    config_data.update(data)
    return jsonify({"message": "Configuration updated", "config": config_data})


@app.route('/api/status', methods=['GET'])
def get_status():
    """Get current system status"""
    return jsonify(system_stats)


@app.route('/api/status/update', methods=['POST'])
def update_status():
    """Update system status (called by MPI application)"""
    global system_stats
    data = request.get_json()
    
    # Update overall status
    if 'status' in data:
        system_stats['status'] = data['status']
    
    # Update worker status
    if 'worker_update' in data:
        worker = data['worker_update']
        for w in system_stats['workers']:
            if w['rank'] == worker['rank']:
                w.update(worker)
                w['last_update'] = datetime.now().isoformat()
                break
    
    # Record steal events
    if 'steal_event' in data:
        event = data['steal_event']
        event['timestamp'] = datetime.now().isoformat()
        system_stats['steal_events'].append(event)
        # Keep only last 100 events
        if len(system_stats['steal_events']) > 100:
            system_stats['steal_events'] = system_stats['steal_events'][-100:]
    
    return jsonify({"message": "Status updated"})


@app.route('/api/worker/<int:rank>', methods=['GET'])
def get_worker(rank):
    """Get specific worker stats"""
    for w in system_stats['workers']:
        if w['rank'] == rank:
            return jsonify(w)
    return jsonify({"error": "Worker not found"}), 404


@app.route('/api/stats/export', methods=['GET'])
def export_stats():
    """Export current stats as JSON"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = STATS_DIR / f"stats_{timestamp}.json"
    
    with open(filename, 'w') as f:
        json.dump(system_stats, f, indent=2)
    
    return jsonify({
        "message": "Stats exported",
        "filename": f"stats_{timestamp}.json"
    })


@app.route('/api/reset', methods=['POST'])
def reset_system():
    """Reset system statistics"""
    global system_stats
    system_stats['status'] = "INITIALIZING"
    system_stats['steal_events'] = []
    system_stats['simulation_elapsed'] = 0.0
    
    for w in system_stats['workers']:
        w['queue_size'] = 0
        w['tasks_processed'] = 0
        w['status'] = 'IDLE'
        w['last_update'] = None
    
    return jsonify({"message": "System reset"})


@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({"status": "online", "timestamp": datetime.now().isoformat()})


# ============================================================================
# Error Handlers
# ============================================================================

@app.errorhandler(404)
def not_found(error):
    return jsonify({"error": "Endpoint not found"}), 404


@app.errorhandler(500)
def server_error(error):
    return jsonify({"error": "Internal server error"}), 500


if __name__ == '__main__':
    print("=" * 70)
    print("HPC LOAD BALANCER DASHBOARD")
    print("=" * 70)
    print("Starting Flask server on http://127.0.0.1:5000")
    print("Press Ctrl+C to stop")
    print("=" * 70)
    app.run(debug=True, host='127.0.0.1', port=5000)
