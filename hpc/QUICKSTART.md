# Quick Start Guide

## 🚀 Getting Started in 3 Steps

### 1️⃣ Build the Project
```bash
cd hpc
make clean && make
```

### 2️⃣ Run a Simulation
```bash
# Default configuration (90k tasks, 8:1 imbalance)
mpirun -np 3 ./bin/load_balancer

# Custom configuration
mpirun -np 3 ./bin/load_balancer -t 50000 -r 4.0 -s 5.0

# Show all options
mpirun -np 3 ./bin/load_balancer -h
```

### 3️⃣ Analyze Results

**Option A: Simple Analysis (No setup required)**
```bash
python3 analyze_metrics_simple.py
```

**Option B: Full Analysis with Plots (One-time setup)**
```bash
# First time only - setup virtual environment
./setup_analysis.sh

# Activate environment
source venv/bin/activate

# Generate visualizations
python analyze_metrics.py

# View generated plots
open metrics_analysis.png
open time_breakdown.png

# When done
deactivate
```

**Option C: Interactive UI Dashboard**
```bash
# One-time dependency install inside your venv
source venv/bin/activate
pip install -r requirements-ui.txt

# Launch dashboard
streamlit run ui_app.py
```

From the UI you can build, run simulations with custom parameters, and view charts from `simulation_metrics.csv`.

---

## 📊 What You'll See

### Console Output
```
====================================================
SIMULATION SUMMARY
====================================================
Total Runtime: 8.017 seconds
Worker 1: 78531 tasks (9739.0 tasks/sec), Idle: 99.7%
Worker 2: 21469 tasks (2651.8 tasks/sec), Idle: 99.9%
Total Throughput: 12473.3 tasks/sec
====================================================
```

### CSV File (`simulation_metrics.csv`)
```csv
worker_id,tasks_processed,steal_attempts,steal_successes,total_time,...
1,78531,2,1,10.094535,...
2,21469,3,2,10.081826,...
```

### Analysis Output
- Load balance statistics
- Throughput per worker
- Idle time analysis
- Steal efficiency metrics
- Visual charts (with full analysis)

---

## 🎯 Common Use Cases

### Test Different Workloads
```bash
# Light workload
mpirun -np 3 ./bin/load_balancer -t 20000 -s 3.0

# Heavy workload
mpirun -np 3 ./bin/load_balancer -t 200000 -s 15.0

# Balanced distribution
mpirun -np 3 ./bin/load_balancer -t 100000 -r 1.0

# Extreme imbalance
mpirun -np 3 ./bin/load_balancer -t 100000 -r 10.0
```

### Scale Testing
```bash
# 2 workers
mpirun -np 3 ./bin/load_balancer -t 100000

# 4 workers
mpirun -np 5 ./bin/load_balancer -t 100000

# 8 workers
mpirun -np 9 ./bin/load_balancer -t 100000
```

### Tuning Parameters
```bash
# Small batches for fine-grained balancing
mpirun -np 3 ./bin/load_balancer -b 256 -w 200

# Large batches for lower overhead
mpirun -np 3 ./bin/load_balancer -b 2048 -w 5000
```

---

## 🔧 Troubleshooting

### "externally-managed-environment" Error
**Solution:** Use the setup script which creates a virtual environment:
```bash
./setup_analysis.sh
source venv/bin/activate
```

### Simulation Ends Too Quickly
```bash
# Increase tasks or time
mpirun -np 3 ./bin/load_balancer -t 200000 -s 20.0
```

### Can't Find CSV File
```bash
# Make sure you're in the hpc directory
cd hpc
ls -l simulation_metrics.csv
```

### Virtual Environment Issues
```bash
# Remove and recreate
rm -rf venv
./setup_analysis.sh
```

---

## 📖 More Information

- **[USAGE.md](USAGE.md)** - Comprehensive usage guide
- **[ENHANCEMENTS.md](ENHANCEMENTS.md)** - Technical details of improvements
- **[README.md](../README.md)** - Project overview

---

## ✨ Key Features

- ✅ Configurable via CLI (no recompilation needed)
- ✅ Smart termination (minimal idle steal attempts)
- ✅ Detailed performance metrics
- ✅ CSV export for data analysis
- ✅ Simple analyzer (no dependencies)
- ✅ Full visualization suite (optional)

Ready to dive deeper? Check out [USAGE.md](USAGE.md)!
