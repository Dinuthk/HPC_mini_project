/**
 * HPC Load Balancer Dashboard - Main JavaScript
 * Handles UI interactions, API calls, and real-time updates
 */

// Global state
let state = {
    config: null,
    status: null,
    charts: {},
    updateInterval: null,
    autoUpdate: true
};

// Chart colors
const COLORS = {
    chart1: '#0066cc',
    chart2: '#00a86b',
    chart3: '#ff6b6b',
    chart4: '#ffa500'
};

// ============================================================================
// Initialization
// ============================================================================

document.addEventListener('DOMContentLoaded', function() {
    console.log('Dashboard Initialized');
    
    // Setup event listeners
    setupNavigationListeners();
    setupButtonListeners();
    setupFormListeners();
    
    // Load initial data
    loadConfiguration();
    loadStatus();
    
    // Start auto-update interval
    startAutoUpdate();
});

// ============================================================================
// Navigation & Tabs
// ============================================================================

function setupNavigationListeners() {
    const navItems = document.querySelectorAll('.nav-item');
    
    navItems.forEach(item => {
        item.addEventListener('click', function(e) {
            e.preventDefault();
            const tabName = this.getAttribute('data-tab');
            switchTab(tabName);
        });
    });
}

function switchTab(tabName) {
    // Hide all tabs
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.classList.remove('active');
    });
    
    // Remove active from nav items
    document.querySelectorAll('.nav-item').forEach(item => {
        item.classList.remove('active');
    });
    
    // Show selected tab
    const selectedTab = document.getElementById(tabName + '-tab');
    if (selectedTab) {
        selectedTab.classList.add('active');
    }
    
    // Mark nav item as active
    const activeNav = document.querySelector(`[data-tab="${tabName}"]`);
    if (activeNav) {
        activeNav.classList.add('active');
    }
}

// ============================================================================
// API Calls
// ============================================================================

async function apiCall(endpoint, method = 'GET', data = null) {
    try {
        const options = {
            method: method,
            headers: {
                'Content-Type': 'application/json'
            }
        };
        
        if (data) {
            options.body = JSON.stringify(data);
        }
        
        const response = await fetch(`/api${endpoint}`, options);
        
        if (!response.ok) {
            throw new Error(`API Error: ${response.statusText}`);
        }
        
        return await response.json();
    } catch (error) {
        console.error('API Call Error:', error);
        showNotification('Error: ' + error.message, 'error');
        return null;
    }
}

async function loadConfiguration() {
    const config = await apiCall('/config');
    if (config) {
        state.config = config;
        updateConfigUI();
    }
}

async function loadStatus() {
    const status = await apiCall('/status');
    if (status) {
        state.status = status;
        updateDashboardUI();
        updateWorkersUI();
        updateEventsUI();
    }
}

// ============================================================================
// UI Updates
// ============================================================================

function updateConfigUI() {
    if (!state.config) return;
    
    document.getElementById('worldSize').textContent = state.config.world_size;
    document.getElementById('numWorkers').textContent = state.config.num_workers;
    
    // Update form fields
    document.getElementById('cfgWorldSize').value = state.config.world_size;
    document.getElementById('cfgNumWorkers').value = state.config.num_workers;
    document.getElementById('cfgInitial1').value = state.config.initial_tasks_node_1;
    document.getElementById('cfgInitial2').value = state.config.initial_tasks_node_2;
    document.getElementById('cfgDuration').value = state.config.simulation_duration;
    document.getElementById('cfgProbeInterval').value = state.config.probe_interval_ms;
}

function updateDashboardUI() {
    if (!state.status) return;
    
    const status = state.status;
    
    // Update status badge
    const statusBadge = document.getElementById('systemStatus');
    statusBadge.querySelector('.status-text').textContent = status.status;
    
    // Update info cards
    document.getElementById('worldSize').textContent = status.world_size;
    document.getElementById('numWorkers').textContent = status.num_workers;
    document.getElementById('statusValue').textContent = status.status;
    
    // Update statistics
    const totalStealEvents = status.steal_events ? status.steal_events.length : 0;
    document.getElementById('totalStealEvents').textContent = totalStealEvents;
    
    let totalTasksMoved = 0;
    if (status.steal_events) {
        status.steal_events.forEach(event => {
            totalTasksMoved += event.tasks_stolen || 0;
        });
    }
    document.getElementById('totalTasksMoved').textContent = totalTasksMoved.toLocaleString();
    document.getElementById('simTime').textContent = (status.simulation_elapsed || 0).toFixed(1) + 's';
    
    // Update charts
    updateCharts();
    
    // Update timestamp
    document.getElementById('lastUpdate').textContent = 'Last updated: ' + new Date().toLocaleTimeString();
}

function updateCharts() {
    if (!state.status || !state.status.workers) return;
    
    const workers = state.status.workers;
    
    // Queue Size Chart
    const queueData = workers.map(w => w.queue_size || 0);
    const queueLabels = workers.map(w => `Worker ${w.rank}`);
    
    if (!state.charts.queue) {
        const queueCtx = document.getElementById('queueChart');
        if (queueCtx) {
            state.charts.queue = new Chart(queueCtx, {
                type: 'bar',
                data: {
                    labels: queueLabels,
                    datasets: [{
                        label: 'Queue Size',
                        data: queueData,
                        backgroundColor: COLORS.chart1,
                        borderRadius: 6,
                        borderSkipped: false
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: true,
                    plugins: {
                        legend: { display: false }
                    },
                    scales: {
                        y: { beginAtZero: true }
                    }
                }
            });
        }
    } else {
        state.charts.queue.data.datasets[0].data = queueData;
        state.charts.queue.update();
    }
    
    // Tasks Processed Chart
    const tasksData = workers.map(w => w.tasks_processed || 0);
    
    if (!state.charts.tasks) {
        const tasksCtx = document.getElementById('tasksChart');
        if (tasksCtx) {
            state.charts.tasks = new Chart(tasksCtx, {
                type: 'doughnut',
                data: {
                    labels: queueLabels,
                    datasets: [{
                        data: tasksData,
                        backgroundColor: [COLORS.chart2, COLORS.chart3],
                        borderColor: white,
                        borderWidth: 2
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: true,
                    plugins: {
                        legend: { position: 'bottom' }
                    }
                }
            });
        }
    } else {
        state.charts.tasks.data.datasets[0].data = tasksData;
        state.charts.tasks.update();
    }
}

function updateWorkersUI() {
    if (!state.status || !state.status.workers) return;
    
    const workers = state.status.workers;
    const container = document.getElementById('workersContainer');
    const tableBody = document.getElementById('workersTableBody');
    
    // Clear containers
    container.innerHTML = '';
    tableBody.innerHTML = '';
    
    workers.forEach(worker => {
        // Create worker card
        const card = createWorkerCard(worker);
        container.appendChild(card);
        
        // Create table row
        const row = createWorkerTableRow(worker);
        tableBody.appendChild(row);
    });
}

function createWorkerCard(worker) {
    const card = document.createElement('div');
    card.className = `worker-card ${worker.status.toLowerCase()}`;
    
    const statusClass = worker.status === 'ACTIVE' ? 'active' : 'idle';
    const timestamp = worker.last_update ? new Date(worker.last_update).toLocaleTimeString() : 'N/A';
    
    card.innerHTML = `
        <div class="worker-header">
            <span class="worker-title">Worker ${worker.rank}</span>
            <span class="worker-badge ${statusClass}">${worker.status}</span>
        </div>
        <div class="worker-body">
            <div class="worker-stat">
                <span class="worker-stat-label">Rank</span>
                <span class="worker-stat-value">${worker.rank}</span>
            </div>
            <div class="worker-stat">
                <span class="worker-stat-label">Status</span>
                <span class="worker-stat-value">${worker.status}</span>
            </div>
            <div class="worker-stat">
                <span class="worker-stat-label">Queue Size</span>
                <span class="worker-stat-value">${(worker.queue_size || 0).toLocaleString()}</span>
            </div>
            <div class="worker-stat">
                <span class="worker-stat-label">Tasks Processed</span>
                <span class="worker-stat-value">${(worker.tasks_processed || 0).toLocaleString()}</span>
            </div>
            <div class="worker-stat">
                <span class="worker-stat-label">Last Update</span>
                <span class="worker-stat-value" style="font-size: 13px;">${timestamp}</span>
            </div>
        </div>
    `;
    
    return card;
}

function createWorkerTableRow(worker) {
    const row = document.createElement('tr');
    const statusClass = worker.status === 'ACTIVE' ? 'active' : 'idle';
    const timestamp = worker.last_update ? new Date(worker.last_update).toLocaleTimeString() : 'N/A';
    
    row.innerHTML = `
        <td>${worker.rank}</td>
        <td>
            <span class="status-badge-table ${statusClass}">${worker.status}</span>
        </td>
        <td>${(worker.queue_size || 0).toLocaleString()}</td>
        <td>${(worker.tasks_processed || 0).toLocaleString()}</td>
        <td>${timestamp}</td>
        <td>
            <button class="btn btn-secondary" onclick="viewWorkerDetails(${worker.rank})">
                <i class="fas fa-eye"></i> View
            </button>
        </td>
    `;
    
    return row;
}

function updateEventsUI() {
    if (!state.status || !state.status.steal_events) return;
    
    const events = state.status.steal_events;
    const container = document.getElementById('eventsContainer');
    
    document.getElementById('eventCount').textContent = events.length;
    
    if (events.length === 0) {
        container.innerHTML = `
            <div class="empty-state">
                <i class="fas fa-inbox"></i>
                <p>No steal events yet. Run the simulation to see load balancing activity.</p>
            </div>
        `;
        return;
    }
    
    container.innerHTML = '';
    
    // Show newest events first
    [...events].reverse().forEach(event => {
        const item = document.createElement('div');
        item.className = 'event-item';
        
        const timestamp = event.timestamp || 'Unknown';
        const source = event.source_rank || 'Unknown';
        const dest = event.dest_rank || 'Unknown';
        const tasks = event.tasks_stolen || 0;
        
        item.innerHTML = `
            <span class="event-timestamp">${new Date(timestamp).toLocaleTimeString()}</span>
            <div class="event-details">
                <div class="event-title">
                    <i class="fas fa-exchange-alt"></i>
                    Work Steal from Node ${source} to Node ${dest}
                </div>
                <div class="event-description">
                    Stole <strong>${tasks.toLocaleString()} tasks</strong> from overloaded node.
                </div>
                <span class="event-badge">Load Balancing</span>
            </div>
        `;
        
        container.appendChild(item);
    });
}

// ============================================================================
// Button Listeners
// ============================================================================

function setupButtonListeners() {
    // Refresh button
    document.getElementById('refreshBtn')?.addEventListener('click', () => {
        loadStatus();
        showNotification('Data refreshed', 'success');
    });
    
    // Reset system button
    document.getElementById('resetBtn')?.addEventListener('click', async () => {
        if (confirm('Are you sure you want to reset the system?')) {
            await apiCall('/reset', 'POST');
            await loadStatus();
            showNotification('System reset', 'success');
        }
    });
    
    // Export stats button
    document.getElementById('exportBtn')?.addEventListener('click', async () => {
        const result = await apiCall('/stats/export', 'GET');
        if (result) {
            showNotification('Stats exported: ' + result.filename, 'success');
        }
    });
    
    // Clear events button
    document.getElementById('clearEventsBtn')?.addEventListener('click', () => {
        if (state.status && state.status.steal_events) {
            state.status.steal_events = [];
            updateEventsUI();
            showNotification('Events cleared', 'success');
        }
    });
}

// ============================================================================
// Form Listeners
// ============================================================================

function setupFormListeners() {
    // World size auto-calculate workers
    document.getElementById('cfgWorldSize')?.addEventListener('change', function() {
        const worldSize = parseInt(this.value);
        document.getElementById('cfgNumWorkers').value = worldSize - 1;
    });
    
    // Save configuration
    document.getElementById('saveCfgBtn')?.addEventListener('click', async () => {
        const newConfig = {
            world_size: parseInt(document.getElementById('cfgWorldSize').value),
            num_workers: parseInt(document.getElementById('cfgNumWorkers').value),
            initial_tasks_node_1: parseInt(document.getElementById('cfgInitial1').value),
            initial_tasks_node_2: parseInt(document.getElementById('cfgInitial2').value),
            simulation_duration: parseFloat(document.getElementById('cfgDuration').value),
            probe_interval_ms: parseInt(document.getElementById('cfgProbeInterval').value)
        };
        
        const result = await apiCall('/config', 'POST', newConfig);
        if (result) {
            showNotification('Configuration saved', 'success');
        }
    });
    
    // Reset configuration
    document.getElementById('resetCfgBtn')?.addEventListener('click', () => {
        updateConfigUI();
        showNotification('Configuration reset to defaults', 'info');
    });
}

// ============================================================================
// Auto Update
// ============================================================================

function startAutoUpdate() {
    state.updateInterval = setInterval(() => {
        if (state.autoUpdate) {
            loadStatus();
        }
    }, 2000); // Update every 2 seconds
}

function stopAutoUpdate() {
    if (state.updateInterval) {
        clearInterval(state.updateInterval);
        state.updateInterval = null;
    }
}

// ============================================================================
// Utilities
// ============================================================================

function showNotification(message, type = 'info') {
    const notification = document.getElementById('notification');
    notification.textContent = message;
    notification.className = `notification show ${type}`;
    
    setTimeout(() => {
        notification.classList.remove('show');
    }, 3000);
}

function viewWorkerDetails(rank) {
    const worker = state.status.workers.find(w => w.rank === rank);
    if (worker) {
        alert(`Worker ${rank} Details:\n\nStatus: ${worker.status}\nQueue: ${worker.queue_size}\nProcessed: ${worker.tasks_processed}`);
    }
}

// Page visibility - pause updates when tab is not visible
document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        state.autoUpdate = false;
    } else {
        state.autoUpdate = true;
    }
});

// On page unload
window.addEventListener('beforeunload', () => {
    stopAutoUpdate();
});
