# Performance Comparison - HPC Load Balancer

Compare the performance of three different implementations:
1. **Sequential** - Single-threaded, no parallelism
2. **Multi-threaded** - Local parallelism with pthreads
3. **MPI** - Distributed, HPC-style load balancing

---

## Overview

This directory contains three implementations of the same task processing workload:

### 1. Sequential Version (sequential.c)
- **Approach**: Single-threaded loop
- **Parallelism**: None
- **Memory**: Shared memory (local)
- **Communication**: None
- **Overhead**: Minimal
- **Best for**: Understanding baseline performance

### 2. Multi-threaded Version (multithreaded.c)
- **Approach**: pthreads with work distribution
- **Parallelism**: Local multi-core (2 threads)
- **Memory**: Shared memory (local)
- **Communication**: Thread synchronization (lightweight)
- **Overhead**: Thread creation, synchronization
- **Best for**: Local multi-core systems

### 3. MPI Version (../hpc/)
- **Approach**: Distributed with load balancing
- **Parallelism**: Network-distributed (3 processes)
- **Memory**: Distributed memory
- **Communication**: Network messages (simulated)
- **Overhead**: Network latency, work-stealing protocol
- **Best for**: Multi-node clusters

---

## Workload Details

All versions process the same workload:

```
Total Tasks:        90,000 (80,000 + 10,000)
Task Complexity:    Medium (100 operations per task)
Task Type:          Trade processing (stock market simulation)
Distribution:       
  - Sequential:       All tasks in single thread
  - Multi-threaded:   Tasks split evenly to threads (45k each)
  - MPI:              Imbalanced load (80k to worker 1, 10k to worker 2)
                      Then dynamically balanced via work-stealing
```

---

## Quick Start

### Step 1: Compile All Versions

```bash
cd comparison
make
```

### Step 2: Run Benchmark

```bash
# Bash/Linux/WSL:
bash benchmark.sh

# Or run individually:
./bin/sequential
./bin/multithreaded
../hpc/bin/load_balancer  # Requires MPI
```

### Step 3: Compare Results

```bash
cat benchmark_results.txt
cat timing_comparison.txt
```

---

## Expected Results

### Typical Output

```
========================================================================
BENCHMARK RESULTS SUMMARY
========================================================================

  Sequential (1 thread):      2.345s
  Multi-threaded (2 threads): 1.234s  (1.90x speedup)
  MPI Distributed (3 proc):   ~6.0s   (load balanced)

========================================================================
ANALYSIS
========================================================================

Sequential vs Multi-threaded:
  ✓ Multi-threaded is FASTER
    Speedup: 1.90x
```

---

## Performance Characteristics

### Sequential
| Metric | Value |
|--------|-------|
| Threads | 1 |
| Memory | ~700 KB |
| Setup Time | <1ms |
| Processing Time | ~1-3s |
| Useful Work | 100% |
| Overhead | ~0% |

### Multi-threaded
| Metric | Value |
|--------|-------|
| Threads | 2 |
| Memory | ~700 KB |
| Setup Time | 1-2ms |
| Processing Time | ~0.5-2s |
| Useful Work | ~80-90% |
| Overhead | ~10-20% (sync) |

### MPI Distributed
| Metric | Value |
|--------|-------|
| Processes | 3 |
| Memory | ~10 MB (total) |
| Setup Time | 100-500ms |
| Processing Time | ~6s |
| Useful Work | ~95%+ |
| Overhead | ~5% (load balancing) |

---

## What to Analyze

### 1. Execution Time
- **Sequential**: Baseline reference
- **Multi-threaded**: Usually 1.5-2x faster (2 threads)
- **MPI**: May be slower (includes network overhead, but shows distributed capability)

### 2. Scalability
- **Sequential**: O(n) - linear with tasks
- **Multi-threaded**: O(n/p) where p = number of threads
- **MPI**: O(n/p) with dynamic load balancing

### 3. Overhead
- **Sequential**: Minimal (~0-1%)
- **Multi-threaded**: Thread overhead (~5-15%)
- **MPI**: Network/coordination overhead (~5-20%)

### 4. Load Balancing
- **Sequential**: N/A (single worker)
- **Multi-threaded**: Static distribution
- **MPI**: Dynamic work-stealing (visible in logs)

---

## File Structure

```
comparison/
├── sequential.c          # Single-threaded version
├── multithreaded.c       # Multi-threaded version
├── Makefile              # Compile all versions
├── benchmark.sh          # Automated benchmark script
├── README.md             # This file
├── bin/                  # Compiled binaries (after make)
│   ├── sequential
│   └── multithreaded
└── benchmark_results.txt # Results file (after benchmark.sh)
```

---

## Manual Compilation

### Compile Sequential Only
```bash
gcc -Wall -Wextra -O3 sequential.c -o sequential
./sequential
```

### Compile Multi-threaded
```bash
gcc -Wall -Wextra -O3 -pthread multithreaded.c -o multithreaded
./multithreaded
```

### Compile MPI (from hpc directory)
```bash
cd ../hpc
make
mpirun -np 3 ./bin/load_balancer
```

---

## Optimization Flags

All versions compiled with:
- `-Wall -Wextra` : Warnings enabled
- `-O3` : Level 3 optimization
- `-pthread` : POSIX threads (when needed)

**Note**: Compilation flags significantly affect performance! The `-O3` flag applies aggressive optimizations.

---

## Understanding the Output

### Sequential Output Example
```
[Initialization] Creating 90000 tasks...
[Initialization Complete] Time: 0.001 seconds

[Processing] Starting sequential task execution...
  Progress: 10000/90000 tasks processed
  Progress: 20000/90000 tasks processed
  ...
[Processing Complete] Tasks Processed: 90000
[Processing Time] 2.345 seconds

[SUMMARY] SEQUENTIAL EXECUTION
Initialization Time:  0.001 seconds
Processing Time:      2.345 seconds
Total Execution Time: 2.346 seconds
Tasks Processed:      90000
Tasks/Second:         38,362
```

### Multi-threaded Output Example
```
[Worker 1] Started - Processing tasks 0 to 44999
[Worker 2] Started - Processing tasks 45000 to 89999
[Worker 1] Completed - Processed 45000 tasks
[Worker 2] Completed - Processed 45000 tasks

[SUMMARY] MULTI-THREADED EXECUTION
Total Execution Time: 1.234 seconds
Tasks Processed:      90000
Tasks/Second:         72,934  (1.90x speedup over sequential)
```

---

## Key Insights

### Why Multi-threaded Might Be Faster
1. Parallelization on multi-core CPU
2. Balanced load distribution
3. Reduced context switching
4. Cache locality improvements

### Why Multi-threaded Might Be Slower
1. Thread creation overhead
2. Synchronization overhead
3. Cache contention between threads
4. False sharing of cache lines

### Why MPI Shows Different Behavior
1. **Setup overhead**: Network initialization
2. **Message passing**: Slower than shared memory
3. **Load balancing**: Work-stealing adds complexity
4. **Network simulation**: May limit performance
5. **Synchronization**: More complex in distributed system

---

## Advanced Analysis

### Calculate Speedup
```bash
Sequential Time:     2.345s
Multi-threaded Time: 1.234s
Speedup = 2.345 / 1.234 = 1.90x
Efficiency = 1.90 / 2 threads = 0.95 (95%)
```

### Identify Bottlenecks
- If sequential is much faster: Communication overhead is issue
- If multi-threaded is much slower: Lock contention or cache issues
- If MPI is much slower: Network latency or protocol overhead

### Modify Parameters
Edit the source files to test:
- **Task count**: Change `total_tasks`
- **Task complexity**: Modify inner loop iterations
- **Number of workers**: Change `num_threads` or mpirun `-np`

---

## Customization

### Change Number of Threads
In `multithreaded.c`:
```c
int num_threads = 2;  // Change this value
```

### Change Number of Tasks
In any file:
```c
int total_tasks = 90000;  // Change this value
```

### Change Task Complexity
In process_trade():
```c
for (int i = 0; i < 100; i++) {  // Change iteration count
    metric = metric * 1.01;
}
```

---

## Troubleshooting

### Compilation Errors
```bash
# Make sure you have gcc and make installed
gcc --version
make --version

# On Ubuntu/Debian:
sudo apt-get install build-essential

# For MPI:
sudo apt-get install openmpi-bin libopenmpi-dev
```

### Permission Denied
```bash
chmod +x benchmark.sh
./benchmark.sh
```

### MPI Not Found
Make sure HPC version is compiled:
```bash
cd ../hpc
make clean && make
cd ../comparison
```

---

## Performance Tips

1. **Disable CPU frequency scaling** for consistent results
2. **Close other applications** to reduce interference
3. **Run multiple times** and average results
4. **Use large task counts** to minimize setup overhead
5. **Check CPU load** - ensure workers can run in parallel

---

## Further Reading

- Intel Threading Guidelines
- POSIX Threads Tutorial
- MPI Performance Optimization
- Memory Hierarchy and Cache Effects

---

## Questions?

Compare the code in each file to see how:
- **Sequential**: Simple loop-based approach
- **Multi-threaded**: Thread creation and distribution
- **MPI**: Message passing and work-stealing

This helps understand the tradeoffs between:
- Simplicity vs Performance
- Local vs Distributed processing
- Synchronization overhead vs Parallelism benefits

---

*Created: March 2026*
*For: HPC Performance Comparison*
