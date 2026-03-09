# HPC Load Balancer - Usage Guide

## Overview
This distributed load balancer simulates work-stealing across MPI worker nodes with configurable parameters and detailed performance metrics.

## Quick Start

### Basic Run (Default Parameters)
```bash
cd hpc
make clean && make
mpirun -np 3 ./bin/load_balancer
```

Defaults:
- Total tasks: 90,000
- Imbalance ratio: 8:1
- Batch size: 1024
- Low watermark: 1000
- Simulation time: 10 seconds

## Command-Line Options

### Syntax
```bash
mpirun -np <N> ./bin/load_balancer [options]
```

### Options

| Flag | Parameter | Description | Default |
|------|-----------|-------------|---------|
| `-t` | `<tasks>` | Total number of tasks to distribute | 90000 |
| `-r` | `<ratio>` | Initial imbalance ratio (e.g., 8.0 = 8:1) | 8.0 |
| `-b` | `<batch>` | Worker processing batch size | 1024 |
| `-w` | `<watermark>` | Queue threshold to trigger steal requests | 1000 |
| `-s` | `<seconds>` | Maximum simulation time | 10.0 |
| `-h` | - | Display help message | - |

## Example Runs

### 1. Balanced Workload (5:1 ratio)
```bash
mpirun -np 3 ./bin/load_balancer -t 100000 -r 5.0
```

### 2. Extreme Imbalance (10:1 ratio)
```bash
mpirun -np 3 ./bin/load_balancer -t 80000 -r 10.0 -s 8.0
```

### 3. Small Tasks with Fast Stealing
```bash
mpirun -np 3 ./bin/load_balancer -t 50000 -w 500 -b 512
```

### 4. Quick Test Run
```bash
mpirun -np 3 ./bin/load_balancer -t 20000 -s 3.0
```

## Output Metrics

### Console Output
The simulation prints:
- Configuration parameters
- Real-time steal events
- Worker completion status
- Summary statistics per worker:
  - Tasks processed
  - Throughput (tasks/sec)
  - Idle time percentage
  - Steal attempts/successes

### CSV Export
Metrics are automatically saved to `simulation_metrics.csv` with columns:

| Column | Description |
|--------|-------------|
| `worker_id` | MPI rank of worker |
| `tasks_processed` | Total tasks completed |
| `steal_attempts` | Number of times worker requested work |
| `steal_successes` | Number of successful work acquisitions |
| `total_time` | Wall-clock time (seconds) |
| `compute_time` | Time spent computing tasks |
| `idle_time` | Time spent waiting for work |
| `throughput` | Tasks per second |

## Performance Analysis

### Key Metrics to Examine

1. **Load Balance Efficiency**
   - Compare `tasks_processed` across workers
   - Lower variance = better load balancing

2. **Steal Effectiveness**
   - `steal_successes / steal_attempts` ratio
   - Higher ratio = more efficient stealing

3. **Idle Time**
   - `idle_time / total_time` percentage
   - Lower percentage = better utilization

4. **Throughput**
   - Total tasks / total time
   - Compare across different configurations

### Example Analysis Workflow

1. Run baseline:
   ```bash
   mpirun -np 3 ./bin/load_balancer -t 100000 -r 8.0 -s 10
   ```

2. Run with different imbalance:
   ```bash
   mpirun -np 3 ./bin/load_balancer -t 100000 -r 4.0 -s 10
   mv simulation_metrics.csv metrics_4_1_ratio.csv
   ```

3. Compare CSV files to analyze impact of imbalance ratio

## Scaling Experiments

### Test Different Numbers of Workers
```bash
# 3 total (1 master + 2 workers)
mpirun -np 3 ./bin/load_balancer -t 100000

# 5 total (1 master + 4 workers)
mpirun -np 5 ./bin/load_balancer -t 100000

# 9 total (1 master + 8 workers)
mpirun -np 9 ./bin/load_balancer -t 100000
```

### Measure Speedup and Efficiency

Calculate:
- **Speedup**: S(p) = T(1) / T(p)
- **Efficiency**: E(p) = S(p) / p

Where:
- T(1) = time with 1 worker
- T(p) = time with p workers
- p = number of workers

## Tips for Research/Benchmarking

1. **Fix random seed** (if needed for reproducibility) - currently deterministic
2. **Run multiple trials** and average results
3. **Monitor system load** (`top`, `htop`) during runs
4. **Vary one parameter at a time** for controlled experiments
5. **Export and plot** CSV data using Python/R/Excel

## Troubleshooting

### Simulation Completes Too Quickly
- Increase `-t` (total tasks)
- Increase `-s` (simulation time)
- Decrease `-b` (batch size) to add overhead

### Too Many Idle Steal Attempts
- Already improved! The global done condition significantly reduces this
- Further reduce by adjusting `-w` (watermark) lower

### Workers Finish at Very Different Times
- Expected with high imbalance ratios
- Check CSV `total_time` to verify
- Lower `-r` for more balanced initial distribution

## Setup for Visualization Tools

### Option 1: Automated Setup (Recommended)

```bash
chmod +x setup_analysis.sh
./setup_analysis.sh
```

### Option 2: Manual Setup

```bash
# Create virtual environment
python3 -m venv venv

# Activate it
source venv/bin/activate

# Install packages
pip install pandas matplotlib

# When done, deactivate
deactivate
```

**Note for macOS Homebrew Python users:** If you get "externally-managed-environment" error, you must use a virtual environment as shown above.

## Generating Plots

After setup, activate the environment and run:

```bash
source venv/bin/activate
python analyze_metrics.py
```

Example Python script to visualize metrics:

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load CSV
df = pd.read_csv('simulation_metrics.csv', comment='#')

# Plot tasks processed
plt.figure(figsize=(10, 6))
plt.bar(df['worker_id'], df['tasks_processed'])
plt.xlabel('Worker ID')
plt.ylabel('Tasks Processed')
plt.title('Load Distribution Across Workers')
plt.savefig('load_distribution.png')
plt.show()

# Plot idle time
plt.figure(figsize=(10, 6))
idle_pct = (df['idle_time'] / df['total_time']) * 100
plt.bar(df['worker_id'], idle_pct)
plt.xlabel('Worker ID')
plt.ylabel('Idle Time (%)')
plt.title('Worker Idle Time Percentage')
plt.savefig('idle_time.png')
plt.show()
```

## What's New in This Version

✅ **CLI Configuration** - All parameters now configurable via command line  
✅ **Global Done Condition** - Simulation ends cleanly when all work is complete  
✅ **Timing Metrics** - Track compute time, idle time, and total time per worker  
✅ **Steal Statistics** - Count attempts vs. successes for analysis  
✅ **CSV Export** - Automatic metrics export for plotting and analysis  
✅ **Throughput Reporting** - Tasks/sec calculated per worker and globally  

## Next Steps

Consider implementing:
- Adaptive chunking (larger chunks when queue is full)
- Non-blocking MPI for lower latency
- Fault tolerance with heartbeats
- More workers (scale to 10+ nodes)
- Comparison with static load distribution
