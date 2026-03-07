# Frontend Dashboard - Complete Index

## 📖 Documentation Index

### Getting Started
1. **[FRONTEND_QUICKSTART.md](FRONTEND_QUICKSTART.md)** ⚡
   - 30-second setup guide
   - Dashboard features overview
   - Quick customization tips
   - Troubleshooting quick fixes
   - **START HERE**

2. **[FRONTEND_OVERVIEW.md](FRONTEND_OVERVIEW.md)** 📋
   - Complete overview of what you received
   - Feature summary
   - Use cases and benefits
   - Getting started checklist
   - Support information

### Detailed Guides
3. **[FRONTEND_INTEGRATION.md](FRONTEND_INTEGRATION.md)** 🔌
   - MPI application integration
   - API endpoint reference
   - Network configuration
   - Security considerations
   - Code examples
   - Docker deployment

4. **[FRONTEND_ARCHITECTURE.md](FRONTEND_ARCHITECTURE.md)** 🏗️
   - System architecture diagrams
   - Component breakdown
   - Data flow documentation
   - File structure details
   - Extension points
   - Performance optimization

### Project Documentation
5. **[frontend/README.md](frontend/README.md)** 📚
   - Full feature documentation
   - Installation instructions
   - Usage guide per tab
   - REST API complete reference
   - Configuration details
   - Troubleshooting guide

---

## 🗂️ project Structure

```
HPC_mini_project/
│
├── 📄 FRONTEND_OVERVIEW.md          ← Complete overview (you are here)
├── 📄 FRONTEND_QUICKSTART.md        ← Quick start (5 min read)
├── 📄 FRONTEND_INTEGRATION.md       ← Integration guide (detailed)
├── 📄 FRONTEND_ARCHITECTURE.md      ← Architecture & design
│
├── frontend/                         ← Dashboard application
│   ├── 📄 app.py                    ← Flask backend server
│   ├── 📄 requirements.txt          ← Python dependencies
│   ├── 📄 README.md                 ← Frontend documentation
│   │
│   ├── templates/
│   │   └── 📄 index.html            ← Dashboard UI (HTML)
│   │
│   ├── static/
│   │   ├── css/
│   │   │   └── 📄 style.css         ← Dashboard styling (CSS)
│   │   │
│   │   └── js/
│   │       └── 📄 main.js           ← Dashboard logic (JavaScript)
│   │
│   └── stats/                        ← Exported stats (auto-created)
│
├── hpc/
│   ├── include/
│   │   └── 📄 dashboard.h           ← Dashboard integration header
│   │
│   └── src/
│       └── 📄 dashboard.c           ← Dashboard integration code
│
└── [existing HPC files...]
```

---

## ✅ What's Included

### Backend (Flask)
- ✅ `app.py` - Complete Flask web server with REST API
- ✅ `requirements.txt` - Python dependencies
- ✅ API endpoints for configuration, status, monitoring, events
- ✅ In-memory data store for real-time updates
- ✅ JSON export capabilities
- ✅ Error handling and health checks

### Frontend (Web UI)
- ✅ `index.html` - Complete dashboard interface
- ✅ `style.css` - Professional responsive design
- ✅ `main.js` - Real-time updates and interactivity
- ✅ 4 main tabs: Dashboard, Workers, Configuration, Events
- ✅ Live charts and statistics
- ✅ Tab navigation and form handling
- ✅ Auto-refresh with smart polling

### MPI Integration
- ✅ `dashboard.h` - C header for dashboard integration
- ✅ `dashboard.c` - Implementation of dashboard functions
- ✅ Simple initialization: `dashboard_init("http://...")`
- ✅ Worker status updates
- ✅ Steal event reporting
- ✅ Non-blocking HTTP requests (curl-based)

### Documentation
- ✅ Quick Start Guide (5 minutes)
- ✅ Integration Guide (detailed, with examples)
- ✅ Architecture Guide (system design)
- ✅ API Reference (complete endpoint docs)
- ✅ README (feature overview)
- ✅ This Index Document

---

## 🚀 Quick Start (Choose One)

### Option 1: View Dashboard Only (No Integration)
```bash
cd frontend
pip install -r requirements.txt
python app.py
# Open http://127.0.0.1:5000
```

### Option 2: Full Integration with MPI App
```bash
# Step 1: Start dashboard
cd frontend
pip install -r requirements.txt
python app.py

# Step 2: In another terminal, compile with dashboard module
cd hpc
make clean && make

# Step 3: Run MPI application
mpirun -np 3 ./bin/load_balancer

# Step 4: View in browser
# Open http://127.0.0.1:5000
```

---

## 📊 Dashboard Features

### Dashboard Tab 📈
- System overview cards (world size, workers, status)
- Live queue distribution chart
- Task processing doughnut chart
- Load balancing statistics
- Real-time metrics

### Workers Tab 👥
- Worker status cards (visual overview)
- Detailed workers table
- Queue size per worker
- Tasks processed per worker
- Last update timestamps
- Status indicators (ACTIVE/IDLE/ERROR)

### Configuration Tab ⚙️
- World size settings
- Task distribution controls
- Simulation duration parameter
- Probe interval configuration
- Save and reset buttons
- Configuration validation

### Events Tab 📋
- Chronological event timeline
- Load balancing steal events
- Source and destination nodes
- Tasks moved per event
- Timestamps
- Event count tracking
- Clear history button

---

## 🔌 Integration Options

### Option A: Automatic (Dashboard Module)
Use the provided C module for easy integration:

```c
#include "dashboard.h"

// In your main()
dashboard_init("http://127.0.0.1:5000");
dashboard_update_worker(rank, queue_size, tasks_processed, status);
dashboard_report_steal_event(source, dest, stolen_count);
dashboard_cleanup();
```

### Option B: Manual (HTTP Requests)
Send curl commands from your application:

```bash
curl -X POST http://127.0.0.1:5000/api/status/update \
  -H "Content-Type: application/json" \
  -d '{"worker_update": {...}}'
```

---

## 📚 Reading Guide

### For First-Time Users
1. Read: **FRONTEND_QUICKSTART.md** (5 min)
2. Run: Start `python app.py`
3. Explore: Click through tabs in browser
4. Check: Configuration tab to understand settings

### For Integration
1. Read: **FRONTEND_INTEGRATION.md** (15 min)
2. Review: Code examples in that guide
3. Copy: `dashboard.h` and `dashboard.c` to your project
4. Implement: Add dashboard calls to your code

### For Understanding System
1. Study: **FRONTEND_ARCHITECTURE.md**
2. Review: File structure and component breakdown
3. Explore: Source code with understanding

### For Complete Reference
1. Check: **frontend/README.md** for detailed docs
2. Test: API endpoints with curl commands
3. Reference: When need specific information

---

## 🎯 Common Tasks

### Task: Start the Dashboard
```bash
cd frontend
python app.py
# Visit http://127.0.0.1:5000
```

### Task: Change Port
Edit `frontend/app.py`:
```python
app.run(debug=True, host='127.0.0.1', port=5001)  # Change 5000 to 5001
```

### Task: Change Refresh Interval
Edit `frontend/static/js/main.js`:
```javascript
state.updateInterval = setInterval(() => { ... }, 5000);  // Change from 2000 to 5000
```

### Task: Modify Colors
Edit `frontend/static/css/style.css`:
```css
:root {
    --primary: #0066cc;  /* Change this color */
    /* ... */
}
```

### Task: Send Test Data
```bash
curl -X POST http://127.0.0.1:5000/api/status/update \
  -H "Content-Type: application/json" \
  -d '{"status": "RUNNING", "worker_update": {"rank": 1, "queue_size": 5000, "tasks_processed": 10000, "status": "ACTIVE"}}'
```

### Task: Export Statistics
```bash
curl http://127.0.0.1:5000/api/stats/export
# File saved in frontend/stats/
```

---

## 🛠️ Development

### File Sizes
- `app.py`: ~250 lines
- `index.html`: ~300 lines
- `style.css`: ~700 lines
- `main.js`: ~600 lines
- `dashboard.c`: ~100 lines
- **Total: ~1,950 lines**

### Dependencies
- Python 3.7+
- Flask 2.3.0
- Flask-CORS 4.0.0
- Chart.js 3.9.1 (CDN - included in HTML)
- Font Awesome 6.4.0 (CDN - included in HTML)

### No Build Required
- No compilation needed
- No npm/webpack
- No database setup
- Just `pip install` and run

---

## 🐛 Troubleshooting

### Dashboard won't start
```bash
# Check Python is installed
python --version

# Check Flask is installed
pip install -r requirements.txt

# Try different port
# Edit app.py, change port 5000 to 5001
```

### Can't connect from browser
```bash
# Check Flask is running
curl http://127.0.0.1:5000/health

# Check firewall/antivirus not blocking port 5000

# If on remote machine, update host in app.py
# app.run(host='0.0.0.0', port=5000)
```

### Dashboard shows empty data
- Normal state - waiting for data
- Send test update: See "Send Test Data" above
- Or run MPI app with dashboard integration

### Charts not displaying
- Check browser console: F12 > Console
- Verify JavaScript enabled
- Check network requests: F12 > Network

---

## 📞 Support Resources

1. **Quick Help**: FRONTEND_QUICKSTART.md
2. **Integration**: FRONTEND_INTEGRATION.md
3. **Architecture**: FRONTEND_ARCHITECTURE.md
4. **Detailed Docs**: frontend/README.md
5. **Code Comments**: Look in source files (app.py, main.js)

---

## 🌟 Key Features

✨ **Real-Time Monitoring**
- Live worker status
- Queue visualization
- Task statistics
- Event tracking

✨ **Configuration Management**
- Easy parameter adjustment
- Validation & feedback
- Save/reset capabilities

✨ **Professional UI**
- Modern responsive design
- Smooth animations
- Mobile-friendly
- Intuitive navigation

✨ **Easy Integration**
- Simple C API
- Minimal code changes
- Non-blocking requests
- Optional (works without it)

✨ **Production Ready**
- Error handling
- Input validation
- Data persistence (optional)
- Scalable architecture

---

## 🎓 Learning Path

1. **Day 1**: Start dashboard, explore UI
2. **Day 2**: Read integration guide, add dashboard module to code
3. **Day 3**: Run simulation with dashboard monitoring
4. **Day 4**: Analyze results, export statistics
5. **Day 5**: Customize and optimize

---

## 📋 Checklist for Setup

- [ ] Read FRONTEND_QUICKSTART.md
- [ ] Navigate to frontend/ directory
- [ ] Run: `pip install -r requirements.txt`
- [ ] Run: `python app.py`
- [ ] Open: http://127.0.0.1:5000
- [ ] Verify: Dashboard loads without errors
- [ ] Explore: Click through all tabs
- [ ] Review: Configuration options
- [ ] (Optional) Read: FRONTEND_INTEGRATION.md
- [ ] (Optional) Add: Dashboard module to MPI code

---

## 💡 Tips & Tricks

1. **Keep browser tab visible** - Dashboard pauses updates when tab hidden
2. **Bookmark the URL** - Quick access to http://127.0.0.1:5000
3. **Use F12 DevTools** - Inspect network requests, logs
4. **Export regularly** - Save statistics before clearing
5. **Customize colors** - Edit --primary variable in style.css
6. **Increase update rate** - Lower interval in main.js for faster updates
7. **Monitor network** - Check network tab (F12) for slow requests

---

## 🚀 Ready to Go!

You now have a complete, production-ready monitoring dashboard for your HPC load balancer.

**Next Step:** `cd frontend && python app.py`

Then visit: **http://127.0.0.1:5000**

Enjoy! 🎉

---

## 📝 Document Legend

| Document | Purpose | Audience | Length |
|----------|---------|----------|--------|
| FRONTEND_QUICKSTART.md | Quick setup | Everyone | 5 min |
| FRONTEND_OVERVIEW.md | Feature overview | Decision makers | 10 min |
| FRONTEND_INTEGRATION.md | Integration guide | Developers | 30 min |
| FRONTEND_ARCHITECTURE.md | System design | Architects/Advanced | 20 min |
| frontend/README.md | Reference | All users | 30 min |
| This Index (INDEX.md) | Navigation | Everyone | 10 min |

---

*Last Updated: March 7, 2026*
*Status: Complete & Ready to Use*
*Version: 1.0.0*
