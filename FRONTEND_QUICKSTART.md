# Frontend Dashboard - Quick Start Guide

## ⚡ 30 Second Setup

### 1. Start the Dashboard Server

```bash
cd frontend
pip install -r requirements.txt
python app.py
```

**Output:**
```
======================================================================
HPC LOAD BALANCER DASHBOARD
======================================================================
Starting Flask server on http://127.0.0.1:5000
Press Ctrl+C to stop
======================================================================
```

### 2. Open in Browser

Visit: **http://127.0.0.1:5000**

## 📊 Dashboard Features

### Dashboard Tab
- **System Overview**: Total processes, master node, worker count
- **Live Metrics**: Real-time charts of queue sizes and tasks
- **Load Balancing Activity**: Steal events and performance stats

### Workers Tab
- Visual status cards for each worker
- Detailed worker statistics table
- Last update timestamps

### Configuration Tab
- Modify system parameters
- Set task distribution
- Control simulation duration
- Changes are immediately reflected

### Steal Events Tab
- Complete timeline of load balancing events
- Source/destination information
- Task counts for each steal operation

## 🔗 Optional: Connect MPI Application

To send real-time updates from your MPI application:

### Option 1: Add Dashboard Module (Easy)

1. **Include header:**
   ```c
   #include "dashboard.h"
   ```

2. **Initialize in main():**
   ```c
   dashboard_init("http://127.0.0.1:5000");
   ```

3. **Send updates from worker/master:**
   ```c
   dashboard_update_worker(rank, queue_size, tasks_processed, status);
   dashboard_report_steal_event(source, dest, tasks_stolen);
   ```

4. **Cleanup on exit:**
   ```c
   dashboard_cleanup();
   ```

### Option 2: Manual HTTP Requests

Send curl commands from `master.c` or `worker.c`:

```bash
curl -X POST http://127.0.0.1:5000/api/status/update \
  -H "Content-Type: application/json" \
  -d '{"worker_update": {"rank": 1, "queue_size": 5000, ...}}'
```

## 📁 File Structure

```
frontend/
├── app.py                      # Flask backend
├── requirements.txt            # Dependencies
├── README.md                   # Full documentation
│
├── templates/
│   └── index.html             # Main dashboard HTML
│
├── static/
│   ├── css/
│   │   └── style.css          # Styling & layout
│   └── js/
│       └── main.js            # Interactivity & API calls
│
└── stats/                      # Exported statistics
```

## 🎯 Key Dashboard Tabs Explained

### Dashboard Tab
Shows real-time overview with:
- World size and worker count
- Current simulation status
- Queue distribution chart
- Task processing statistics
- Total steal events

### Workers Tab
Monitor individual worker nodes:
- **Card View**: Visual status with color coding
- **Table View**: Detailed metrics and timestamps
- **Status Indicators**:
  - 🟢 ACTIVE: Currently processing tasks
  - 🟡 IDLE: Waiting for work
  - 🔴 ERROR: Problem detected

### Configuration Tab
Adjust system settings:
- World size (requires rebuild)
- Initial task distribution
- Simulation duration
- Probe interval between checks
- Save/Reset configurations

### Steal Events Tab
Historical record of load balancing:
- Chronological event timeline
- Source and destination nodes
- Number of tasks transferred
- Real-time updates as events occur

## 🚀 Running Full Integration

### Terminal 1: Start Dashboard
```bash
cd frontend
python app.py
```

### Terminal 2: Run MPI Application
```bash
cd hpc
mpirun -np 3 ./bin/load_balancer
```

### Terminal 3: View Dashboard
Open browser at: http://127.0.0.1:5000

## 📈 What to Expect

Once running, the dashboard will show:

```
Status: RUNNING
Total World Size: 3
Master Node: Rank 0
Number of Workers: 2

Worker 1 Queue: 80000 tasks
Worker 2 Queue: 10000 tasks

[Watch as load balancer redistributes tasks across workers]

Load Balancing Events:
- Worker 2 steals 5000 tasks from Worker 1
- Worker 1 steals 10000 tasks from Worker 2
- ...continuing until balanced...
```

## ⚙️ Common Customizations

### Change Port
Edit `frontend/app.py`:
```python
app.run(debug=True, host='127.0.0.1', port=5001)  # Change 5000 to 5001
```

### Change Colors
Edit `frontend/static/css/style.css`:
```css
:root {
    --primary: #0066cc;      /* Change this to your color */
    --success: #28a745;
    /* ... other colors ... */
}
```

### Auto-Update Frequency
Edit `frontend/static/js/main.js`:
```javascript
state.updateInterval = setInterval(() => {
    if (state.autoUpdate) {
        loadStatus();
    }
}, 2000); // Change from 2000ms to desired interval
```

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| "Connection refused" | Start Flask: `python app.py` |
| Port 5000 in use | Change port in `app.py` or kill process using `lsof -i :5000` |
| Dashboard empty | Dashboard needs API calls from MPI app or manual data entry |
| Charts don't show | Check browser console (F12) for errors |
| Slow updates | Reduce auto-update interval in `main.js` |

## 📚 Full Documentation

- [FRONTEND_INTEGRATION.md](../FRONTEND_INTEGRATION.md) - Integration with MPI app
- [frontend/README.md](README.md) - Complete API reference
- [Frontend Dashboard API](#) - API endpoint documentation

## 🎨 UI Preview

### Dashboard Layout
```
┌─────────────────────────────────────────────────────────┐
│  HPC Load Balancer         [Status: RUNNING] [Refresh]  │
├──────────────┬─────────────────────────────────────────┤
│  Dashboard   │  System Overview                         │
│  Workers     │  ┌──────┬──────┬──────┬──────┐          │
│  Config      │  │World │Master│Worker│Status│          │
│  Events      │  │  3   │  0   │  2   │RUNNING          │
│              │  └──────┴──────┴──────┴──────┘          │
│              │                                          │
│              │  Live Metrics                            │
│              │  [Queue Distribution] [Tasks Processed]  │
│              │                                          │
│              │  Load Balancing Activity                 │
│              │  Steal Events: 15 | Tasks Moved: 50k     │
└──────────────┴─────────────────────────────────────────┘
```

## 🎓 Learning Path

1. **Start with Dashboard**: Understand the UI
2. **Read Configuration**: Learn parameter meanings
3. **Review Events**: See load balancing in action
4. **Check Integration Guide**: Connect to MPI app
5. **Customize**: Adjust colors, intervals, settings

## 🔧 API Endpoints Quick Reference

- `GET /api/config` - Get configuration
- `POST /api/config` - Update configuration
- `GET /api/status` - Get system status
- `POST /api/status/update` - Send status update
- `GET /api/worker/<rank>` - Get worker details
- `POST /api/reset` - Reset system
- `GET /api/stats/export` - Export stats as JSON
- `GET /health` - Health check

## 📱 Mobile & Responsive

The dashboard is responsive and works on:
- ✅ Desktop browsers (Chrome, Firefox, Safari, Edge)
- ✅ Tablets (iPad, Android tablets)
- ✅ Mobile phones (iOS Safari, Chrome Mobile)

## 🌟 Pro Tips

1. **Use Chrome DevTools** (F12) to monitor network requests
2. **Set visualization update frequency** based on simulation speed
3. **Export statistics** frequently to build historical data
4. **Compare configurations** by exporting before/after stats
5. **Monitor tab** keeps running - pause with visibility API

## 📞 Still Need Help?

- Check browser console for errors: Press `F12`
- Verify Flask is running: `curl http://127.0.0.1:5000/health`
- Review network requests in DevTools: Network tab
- Check MPI app output for any errors

---

**Ready to go?** Start with: `python app.py` and open http://127.0.0.1:5000 🚀
