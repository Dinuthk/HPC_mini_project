# Frontend Architecture & Components

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        USER INTERFACE (Browser)                     │
│                     http://127.0.0.1:5000                           │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │  Dashboard | Workers | Configuration | Events                │  │
│  │                                                               │  │
│  │  • System Overview Stats                                      │  │
│  │  • Real-time Charts (Queue, Tasks)                           │  │
│  │  • Worker Status Cards                                       │  │
│  │  • Configuration Forms                                       │  │
│  │  • Event Timeline                                            │  │
│  └───────────────────────────────────────────────────────────────┘  │
└────────────────────────────┬────────────────────────────────────────┘
                             │
                     HTTP/REST API Calls
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     FLASK BACKEND (app.py)                          │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  REST API Endpoints                                          │  │
│  │  • GET  /api/config - Retrieve configuration                │  │
│  │  • POST /api/config - Update configuration                  │  │
│  │  • GET  /api/status - Get full system status                │  │
│  │  • POST /api/status/update - Update from MPI app            │  │
│  │  • GET  /api/worker/<rank> - Get worker details             │  │
│  │  • POST /api/reset - Reset statistics                       │  │
│  │  • GET  /api/stats/export - Export as JSON                  │  │
│  │  • GET  /health - Health check                              │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  In-Memory Data Store                                        │  │
│  │  • system_stats: Full system state                           │  │
│  │  • config_data: Configuration settings                       │  │
│  │  • steal_events: Last 100 load-balancing events              │  │
│  └──────────────────────────────────────────────────────────────┘  │
└────────────────────────────┬────────────────────────────────────────┘
                             │
                    HTTP/curl Status Updates
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    MPI DISTRIBUTED APPLICATION                      │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Master Node (Rank 0)                                        │  │
│  │  • Orchestrates load balancing                               │  │
│  │  • Monitors worker status                                    │  │
│  │  • Initiates work stealing                                   │  │
│  │  • Reports status to dashboard                               │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Worker Nodes (Rank 1, 2, ...)                               │  │
│  │  • Process tasks from queue                                  │  │
│  │  • Send status updates periodically                          │  │
│  │  • Respond to work-stealing requests                         │  │
│  │  • Report queue and processing stats                         │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

## 📂 File Structure

```
frontend/
│
├── 📄 app.py (≈250 lines)
│   └─ Flask web server
│   └─ REST API endpoints
│   └─ System state management
│   └─ Configuration handling
│
├── 📄 requirements.txt
│   └─ Python dependencies
│
├── 📄 README.md
│   └─ Full documentation & API reference
│
├── 📁 templates/
│   └── 📄 index.html (≈300 lines)
│       └─ Main HTML structure
│       └─ Navigation sidebar
│       └─ Tab content sections
│       └─ Chart containers
│       └─ Form controls
│       └─ Event timeline
│
├── 📁 static/
│   │
│   ├── 📁 css/
│   │   └── 📄 style.css (≈700 lines)
│   │       └─ CSS variables (colors, spacing)
│   │       └─ Layout & grid system
│   │       └─ Component styling
│   │       └─ Responsive design
│   │       └─ Animations
│   │
│   └── 📁 js/
│       └── 📄 main.js (≈600 lines)
│           └─ Global state management
│           └─ API communication
│           └─ Chart initialization/updates
│           └─ Tab navigation
│           └─ Real-time updates
│           └─ Event listeners
│
└── 📁 stats/ (auto-created)
    └─ JSON export files
```

## 🔄 Data Flow

### 1. **Initial Load**
```
Browser → Flask GET /api/config → Database → Browser
Browser → Flask GET /api/status → Database → Browser
```

### 2. **Dashboard Updates (Every 2 seconds)**
```
JavaScript Timer → API GET /api/status → Flask
Flask → In-Memory Data → Response JSON
Browser ← Update Charts & Cards
```

### 3. **Configuration Changes**
```
User ← Form Submit → Browser JavaScript
JavaScript → API POST /api/config
Flask ← Update config_data
Flask → Response ← Browser
Browser ← Show confirmation
```

### 4. **MPI Application Status**
```
Master/Worker (C) → curl HTTP Request
API POST /api/status/update
Flask ← JSON Update Data
Flask → Update system_stats
Browser ← Auto-refresh shows new data
```

### 5. **Event Recording**
```
Master (C) ← Detects steal opportunity
Master → curl with steal_event data
API POST /api/status/update
Flask ← Store event in steal_events list
Browser ← Event appears in timeline (next refresh)
```

## 🎨 Component Architecture

### Frontend Components
```
index.html
├── Header Component
│   ├── Title & Logo
│   ├── Status Badge
│   └── Refresh Button
│
├── Sidebar Navigation
│   ├── Nav Items (Dashboard, Workers, Config, Events)
│   └── Footer Buttons (Reset, Export)
│
└── Main Content Area
    ├── Dashboard Tab
    │   ├── Info Grid
    │   ├── Metrics Charts
    │   └── Statistics Boxes
    │
    ├── Workers Tab
    │   ├── Worker Cards
    │   └── Workers Table
    │
    ├── Configuration Tab
    │   ├── Form Sections
    │   └── Action Buttons
    │
    └── Events Tab
        ├── Event Controls
        └── Event Timeline
```

### JavaScript Modules
```
main.js
├── Global State Management
│   └── system_stats, charts, config, updateInterval
│
├── Initialization
│   ├── setupNavigationListeners()
│   ├── setupButtonListeners()
│   ├── setupFormListeners()
│   └── startAutoUpdate()
│
├── API Communication
│   ├── apiCall()
│   ├── loadConfiguration()
│   ├── loadStatus()
│   └── POST/GET handlers
│
├── UI Updates
│   ├── updateConfigUI()
│   ├── updateDashboardUI()
│   ├── updateCharts()
│   ├── updateWorkersUI()
│   ├── updateEventsUI()
│   └── createWorkerCard()
│
├── Event Handlers
│   ├── Tab switching
│   ├── Button clicks
│   ├── Form submissions
│   └── Auto-updates
│
└── Utilities
    ├── showNotification()
    ├── viewWorkerDetails()
    └── Page visibility handling
```

### Flask Modules
```
app.py
├── Configuration (5-10 lines)
│   └── WORLD_SIZE, NUM_WORKERS, paths
│
├── Global State (10-20 lines)
│   ├── system_stats
│   ├── config_data
│   └── STATS_DIR
│
├── API Routes (≈200 lines)
│   ├── index()            - Serve HTML
│   ├── get_config()       - GET /api/config
│   ├── update_config()    - POST /api/config
│   ├── get_status()       - GET /api/status
│   ├── update_status()    - POST /api/status/update
│   ├── get_worker()       - GET /api/worker/<rank>
│   ├── export_stats()     - GET /api/stats/export
│   ├── reset_system()     - POST /api/reset
│   └── health()           - GET /health
│
├── Error Handlers (10-15 lines)
│   ├── not_found()        - 404
│   └── server_error()     - 500
│
└── Startup (5-10 lines)
    └── app.run() initialization
```

## 📊 Chart Library

Uses **Chart.js 3.9.1** from CDN for:
- **Bar Charts**: Worker queue sizes
- **Doughnut Charts**: Task distribution
- **Responsive**: Auto-scales to container
- **Real-time**: Updates via chart.update()

## 🔄 State Management

### Global State Object
```javascript
state = {
    config: {
        world_size: 3,
        num_workers: 2,
        initial_tasks_node_1: 80000,
        initial_tasks_node_2: 10000,
        simulation_duration: 6.0,
        probe_interval_ms: 20
    },
    
    status: {
        world_size: 3,
        workers: [
            { rank: 1, status: "ACTIVE", queue_size: 5000, ... },
            { rank: 2, status: "IDLE", queue_size: 0, ... }
        ],
        steal_events: [ ... ]
    },
    
    charts: {
        queue: Chart instance,
        tasks: Chart instance
    },
    
    updateInterval: setInterval ID,
    autoUpdate: boolean
}
```

## 🔐 Security Features

- **Input Validation**: All POST data validated
- **Error Handling**: Graceful error display
- **Page Visibility API**: Pauses updates when tab not visible
- **CORS Disabled**: Same-origin only (default)
- **No Authentication**: Suitable for localhost/protected networks

## ⚡ Performance Optimizations

1. **Lazy Chart Initialization**: Charts created only when needed
2. **Efficient DOM Updates**: Only update changed elements
3. **Background Updates**: HTTP requests don't block UI
4. **Event Throttling**: Last 100 events kept in memory
5. **CSS Animations**: GPU-accelerated transitions
6. **Responsive Images**: No unnecessary image scaling

## 🚀 Scaling Considerations

### Current Limits
- Suitable for up to ~10-20 worker nodes
- Dashboard updates every 2 seconds
- Keeps ~100 events in memory
- No persistence (data lost on restart)

### For Large-Scale (>50 nodes)
Consider:
- Database backend (PostgreSQL, MongoDB)
- Implement pagination for events
- Add filtering/search capabilities
- Increase update intervals
- Implement data aggregation

## 📈 Extension Points

### Easy Extensions
1. **Add new charts**: Create new Canvas element + Chart.js instance
2. **New API endpoint**: Add route in Flask app
3. **Custom styling**: Modify CSS variables
4. **Additional tabs**: Add tab markup + navigation handler

### Advanced Extensions
1. **Database integration**: Replace in-memory store with DB
2. **Authentication**: Add login page + session management
3. **HTTPS/SSL**: Deploy with Gunicorn + reverse proxy
4. **Multiple systems**: Monitor multiple clusters simultaneously
5. **Notifications**: Email/Slack alerts for anomalies

---

**Total Lines of Code:**
- HTML: ~300
- CSS: ~700
- JavaScript: ~600
- Python: ~250
- **Total: ~1,850 lines**

**All components are modular and independently maintainable.**
