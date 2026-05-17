# HPC Mini Project — Distributed Load Balancer Simulation

A high-performance computing project that demonstrates **dynamic work-stealing load balancing** using MPI, OpenMP, and Pthreads. Includes a sequential baseline for performance comparison and a web dashboard for control.

---

## Project Structure

```
HPC_mini_project/
├── hpc/                        # HPC Load Balanced Version
│   ├── src/
│   │   ├── main.c              # MPI init, queue implementations, routing
│   │   ├── master.c            # Master node — scheduling, work-stealing orchestration
│   │   ├── worker.c            # Worker node — queue processing, idle detection
│   │   ├── listener.c          # Pthread background thread — async MPI listener
│   │   ├── compute.c           # OpenMP CPU kernel — Monte Carlo risk simulation
│   │   ├── config.c            # System configuration display
│   │   └── dashboard.c         # Optional HTTP dashboard integration
│   ├── include/
│   │   ├── common.h            # Structs (Trade, TradeQueue), MPI tags, shared globals
│   │   ├── config.h            # Configurable parameters (task counts, duration)
│   │   ├── master.h            # Master function declarations
│   │   ├── worker.h            # Worker function declarations
│   │   ├── listener.h          # Listener function declarations
│   │   ├── compute.h           # Compute function declarations
│   │   └── dashboard.h         # Dashboard function declarations
│   ├── Makefile                # Build with mpicc + OpenMP + pthreads + math
│   ├── bin/                    # Compiled binary (load_balancer)
│   └── obj/                    # Object files
│
├── no_balancer/                # Sequential Baseline (No HPC)
│   ├── no_balancer.c           # Single-threaded, no MPI/OpenMP/Pthreads
│   ├── Makefile                # Build with plain gcc
│   └── bin/                    # Compiled binary (no_balancer)
│
├── dashboard/                  # Web UI Dashboard
│   ├── app.py                  # Flask backend — parameter control, process management
│   ├── templates/
│   │   └── index.html          # Premium dark-themed UI
│   └── requirements.txt        # Python dependencies (Flask)
│
├── .gitignore
└── README.md
```

---

## Prerequisites

- **WSL Ubuntu** with the following installed:
  - `mpicc` / `mpirun` (OpenMPI)
  - `gcc`
  - `make`
- **Python 3** (Windows) with `pip` for the dashboard

### Install MPI on Ubuntu (WSL)

```bash
sudo apt update
sudo apt install openmpi-bin libopenmpi-dev build-essential
```

---

## How to Run

### 1. HPC Load Balanced Version

Uses **MPI** (3 processes: 1 master + 2 workers), **OpenMP** (multi-core parallelism), and **Pthreads** (async network listener). Dynamic work-stealing redistributes tasks from overloaded to idle workers.

```bash
# Inside WSL Ubuntu
cd /mnt/d/Github_work_place/HPC_mini_project/hpc
make clean && make && mpirun --allow-run-as-root -np 3 ./bin/load_balancer
```

### 2. No Balancer — Sequential Baseline

Pure **single-threaded** C — no MPI, no OpenMP, no Pthreads. Each worker's tasks are processed one after another. Demonstrates the inefficiency of static assignment without load balancing.

```bash
# Inside WSL Ubuntu
cd /mnt/d/Github_work_place/HPC_mini_project/no_balancer
make clean && make && ./bin/no_balancer
```

### 3. Web Dashboard (GUI Control)

A Flask-powered web UI to control both simulations, modify parameters, and view real-time terminal output.

```bash
# From Windows PowerShell or CMD
cd d:\Github_work_place\HPC_mini_project\dashboard
pip install -r requirements.txt
python app.py
```

Then open **http://127.0.0.1:5000** in your browser.

---

## Configurable Parameters

### HPC Version

| Parameter | File | Default | Description |
|---|---|---|---|
| `simulations` | `hpc/src/compute.c` | 50,000 | Monte Carlo iterations per trade |
| `INITIAL_TASKS_NODE_1` | `hpc/include/config.h` | 500,000 | Tasks assigned to Worker 1 |
| `INITIAL_TASKS_NODE_2` | `hpc/include/config.h` | 100,000 | Tasks assigned to Worker 2 |
| `SIMULATION_DURATION_SECONDS` | `hpc/include/config.h` | 360.0 | Max duration timeout (seconds) |
| `-np` | CLI argument | 3 | MPI processes (1 master + N workers) |

### No Balancer Version

| Parameter | File | Default | Description |
|---|---|---|---|
| `simulations` | `no_balancer/no_balancer.c` | 50,000 | Monte Carlo iterations per trade |
| `TASKS_WORKER_1` | `no_balancer/no_balancer.c` | 500,000 | Tasks for Worker 1 |
| `TASKS_WORKER_2` | `no_balancer/no_balancer.c` | 100,000 | Tasks for Worker 2 |

> **Note:** All parameters can be modified directly from the web dashboard UI without editing source files.

---

## HPC Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     MASTER NODE (Rank 0)                     │
│                                                              │
│  • Distributes initial tasks (imbalanced: 500k vs 100k)     │
│  • Monitors TAG_IDLE requests from workers                   │
│  • Orchestrates work-stealing protocol                       │
│  • Detects completion and sends shutdown signals             │
└────────────────────┬────────────────┬────────────────────────┘
                     │                │
            TAG_WORK │    TAG_STEAL   │ TAG_WORK
                     │                │
  ┌──────────────────▼──┐   ┌────────▼───────────────────┐
  │   WORKER 1 (Rank 1) │   │   WORKER 2 (Rank 2)       │
  │                      │   │                            │
  │  ┌─── Main Thread ─┐│   │  ┌─── Main Thread ────┐   │
  │  │ • Pop from queue ││   │  │ • Pop from queue   │   │
  │  │ • OpenMP compute ││   │  │ • OpenMP compute   │   │
  │  │ • Detect idle    ││   │  │ • Detect idle      │   │
  │  └──────────────────┘│   │  └────────────────────┘   │
  │                      │   │                            │
  │  ┌── Pthread ──────┐ │   │  ┌── Pthread ──────────┐  │
  │  │ • Async listener│ │   │  │ • Async listener    │  │
  │  │ • Receive work  │ │   │  │ • Receive work      │  │
  │  │ • Surrender work│ │   │  │ • Surrender work    │  │
  │  └─────────────────┘ │   │  └─────────────────────┘  │
  └──────────────────────┘   └────────────────────────────┘
```

### MPI Communication Tags

| Tag | Name | Purpose |
|---|---|---|
| 1 | `TAG_WORK` | Send/receive trade batches |
| 2 | `TAG_KILL_SIGNAL` | Shutdown signal from master |
| 3 | `TAG_IDLE` | Worker reports starvation |
| 4 | `TAG_STEAL_REQ` | Master orders task surrender |
| 5 | `TAG_STOLEN_WORK` | Worker returns stolen tasks |

### Work-Stealing Protocol

1. Worker's queue drops below `LOW_WATERMARK` (1000 tasks)
2. Worker sends `TAG_IDLE` to Master
3. Master sends `TAG_STEAL_REQ` to the other worker
4. Other worker surrenders **half its queue** from the rear (avoiding conflicts with front-processing)
5. Master forwards stolen tasks to the starving worker
6. If 0 tasks stolen from both workers → all work complete → early termination

---

## Compute Kernel

Both versions use the **identical** Black-Scholes / Monte Carlo risk simulation:

```c
for (int i = 0; i < simulations; i++) {
    double pseudo_rand = (double)(i % 100) / 100.0;
    double drift = (base_price * 0.02) - (volume * 0.00005);
    double volatility = sin(base_price * pseudo_rand) * cos(volume * 0.001) * weight;
    double simulated_price = base_price * exp(drift + volatility + log(1.0 + pseudo_rand));
    total_simulated_risk += simulated_price;
}
```

**Per-trade cost:** 50,000 iterations × `sin()` + `cos()` + `exp()` + `log()` = heavy CPU load.

| | HPC | No Balancer |
|---|---|---|
| Parallelism | `#pragma omp parallel for` across all CPU cores | None — single thread |
| Total tasks | 600,000 | 600,000 |
| Work distribution | Dynamic (rebalanced via stealing) | Static (fixed, no redistribution) |

---

## Expected Results

- **HPC version**: Both workers finish at roughly the same time due to dynamic load balancing. Total time is significantly faster.
- **No Balancer**: Worker 2 finishes its 100k tasks quickly, then sits **completely idle** while Worker 1 grinds through 500k alone on a single thread. Much slower overall.

---

## Technologies Used

| Technology | Role |
|---|---|
| **MPI (OpenMPI)** | Inter-process communication across distributed nodes |
| **OpenMP** | Shared-memory multi-core parallelism for compute kernel |
| **Pthreads** | Asynchronous background listener thread per worker |
| **C (gcc/mpicc)** | Core simulation code |
| **Python (Flask)** | Web dashboard backend |
| **HTML/CSS/JS** | Dashboard frontend UI |
