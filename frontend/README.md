# HPC Load Balancer Dashboard

A modern web-based monitoring and control interface for the Distributed Load Balancer simulation.

## Features

- **Real-time Dashboard**: Live monitoring of worker nodes, task queues, and load balancing activity
- **System Configuration**: Easy configuration of world size, task distribution, and simulation parameters
- **Worker Status Panel**: Detailed view of each worker node's status and performance metrics
- **Event Timeline**: Complete history of load balancing (work-stealing) events
- **Interactive Charts**: Visualize queue sizes and task distribution across workers
- **REST API**: Full API for integration with the MPI application

## Directory Structure

```
frontend/
├── app.py                    # Flask web server
├── requirements.txt          # Python dependencies
├── README.md                 # This file
├── templates/
│   └── index.html           # Main dashboard HTML
├── static/
│   ├── css/
│   │   └── style.css        # Dashboard styling
│   └── js/
│       └── main.js          # JavaScript interactivity
└── stats/                   # Exported statistics (auto-created)
```

## Prerequisites

- Python 3.7+
- pip (Python package manager)

## Installation

1. **Navigate to the frontend directory:**
```bash
cd frontend
```

2. **Create a Python virtual environment (optional but recommended):**
```bash
python -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
```

3. **Install dependencies:**
```bash
pip install -r requirements.txt
```

## Running the Dashboard

1. **Start the Flask web server:**
```bash
python app.py
```

2. **Open your web browser:**
```
http://127.0.0.1:5000
```

The dashboard will now be available at `http://localhost:5000`

## Usage

### Dashboard Tab
- **System Overview**: Displays total world size, master rank, and number of workers
- **Live Metrics**: Real-time charts showing queue distribution and task processing
- **Load Balancing Activity**: Summary of steal events and tasks moved

### Workers Tab
- **Worker Cards**: Visual status of each worker node
- **Details Table**: Comprehensive statistics for each worker
- Individual worker performance metrics

### Configuration Tab
- **Basic Settings**: Configure world size and number of workers
- **Task Distribution**: Set initial task loads for each node
- **Simulation Parameters**: Control simulation duration and probe intervals
- **Save/Reset**: Apply or revert configuration changes

### Steal Events Tab
- **Event Timeline**: Chronological list of all load balancing events
- **Event Details**: Source/destination nodes and tasks moved
- **Event Count**: Total number of steal events

## API Endpoints

The dashboard exposes the following REST API endpoints:

### Configuration
- `GET /api/config` - Retrieve current configuration
- `POST /api/config` - Update configuration

### System Status
- `GET /api/status` - Get complete system status
- `POST /api/status/update` - Update system status (from MPI app)
- `GET /api/worker/<rank>` - Get specific worker stats

### Operations
- `POST /api/reset` - Reset system statistics
- `GET /api/stats/export` - Export current stats as JSON
- `GET /health` - Health check endpoint

## Integration with MPI Application

The MPI load balancer application can send status updates to the dashboard via HTTP POST requests:

### Example: Update Status
```bash
curl -X POST http://127.0.0.1:5000/api/status/update \
  -H "Content-Type: application/json" \
  -d '{
    "status": "RUNNING",
    "worker_update": {
      "rank": 1,
      "queue_size": 5000,
      "tasks_processed": 10000,
      "status": "ACTIVE"
    }
  }'
```

### Example: Record Steal Event
```bash
curl -X POST http://127.0.0.1:5000/api/status/update \
  -H "Content-Type: application/json" \
  -d '{
    "steal_event": {
      "source_rank": 2,
      "dest_rank": 1,
      "tasks_stolen": 5000
    }
  }'
```

## Features

### Real-time Updates
- Dashboard automatically refreshes every 2 seconds
- Respects page visibility (pauses updates when tab is not in focus)

### Data Export
- Export system statistics as JSON
- Timestamped files saved in `stats/` directory

### Responsive Design
- Works on desktop, tablet, and mobile devices
- Sidebar navigation collapses on smaller screens
- Touch-friendly interface

## Customization

### Styling
Edit `static/css/style.css` to customize colors, fonts, and layout.

### Colors (CSS Variables)
```css
--primary: #0066cc          /* Main accent color */
--secondary: #6c757d        /* Secondary text */
--success: #28a745          /* Success status */
--danger: #dc3545           /* Error/danger states */
--warning: #ffc107          /* Warning states */
```

### Update Interval
Modify the refresh interval in `static/js/main.js`:
```javascript
state.updateInterval = setInterval(() => {
    if (state.autoUpdate) {
        loadStatus();
    }
}, 2000); // Change this value (milliseconds)
```

## Troubleshooting

### Dashboard shows "Initializing"
- Ensure the Flask server is running
- Check that the MPI application hasn't sent status updates yet

### Port 5000 already in use
Edit `app.py` and change the port:
```python
app.run(debug=True, host='127.0.0.1', port=5001)
```

### Charts not displaying
- Ensure JavaScript is enabled in your browser
- Check browser console for errors (F12 > Console tab)
- Verify Chart.js library is loaded (CDN might be blocked)

### API calls failing
- Check browser Network tab (F12 > Network)
- Verify Flask server is running on correct address/port
- Check CORS configuration if accessing from different origin

## Performance Notes

- Dashboard updates every 2 seconds (adjustable)
- Keeps only last 100 steal events in memory
- Suitable for monitoring small to medium-sized clusters
- For large-scale deployments, consider database backend

## Future Enhancements

- Database backend for persistent storage
- Authentication and user management
- Advanced filtering and search
- Historical analytics and reports
- Custom alert thresholds
- Email/Slack notifications

## License

Same as parent HPC project

## Support

For issues or questions, refer to the main project README.
