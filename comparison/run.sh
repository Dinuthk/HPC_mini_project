#!/bin/bash

# Quick run script - Compile and run all comparison versions

cd "$(dirname "$0")"

echo "========================================================================"
echo "HPC LOAD BALANCER - PERFORMANCE COMPARISON"
echo "========================================================================"
echo ""

# Compile
echo "[1] Compiling versions..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "Error: Compilation failed"
    exit 1
fi

echo "✓ Compilation complete"
echo ""

# Run Sequential
echo "[2] Running Sequential (single-threaded)..."
echo "---"
time ./bin/sequential
echo ""

# Run Multi-threaded
echo "[3] Running Multi-threaded (pthreads)..."
echo "---"
time ./bin/multithreaded
echo ""

echo "========================================================================"
echo "Comparison complete!"
echo ""
echo "✓ Sequential version:     ./bin/sequential"
echo "✓ Multi-threaded version: ./bin/multithreaded"
echo "✓ MPI version:            ../hpc/bin/load_balancer"
echo ""
echo "To run MPI version:"
echo "  mpirun -np 3 ../hpc/bin/load_balancer"
echo ""
echo "========================================================================"
