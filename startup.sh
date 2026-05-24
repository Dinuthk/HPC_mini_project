#!/bin/bash
# Startup script for HPC Dashboard + Backend

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
HPC_DIR="$PROJECT_ROOT/hpc"
DASHBOARD_DIR="$PROJECT_ROOT/dashboard"

echo "======================================================"
echo "HPC Load Balancer + CUDA Dashboard Startup"
echo "======================================================"
echo ""

# Step 1: Build HPC backend
echo "[Step 1/3] Building HPC backend..."
cd "$HPC_DIR"
make clean >/dev/null 2>&1
echo "  Compiling..."
make 2>&1 | grep -E "(CUDA support|error:|Error:|undefined reference)" || true

if [ ! -f "bin/load_balancer" ]; then
    echo "  ✗ FAILED: Binary not created"
    exit 1
fi
echo "  ✓ Binary ready: $(ls -lh bin/load_balancer | awk '{print $5, $9}')"
echo ""

# Step 2: Show run instructions
echo "[Step 2/3] Ready to run"
echo ""
echo "Dashboard will run on: http://127.0.0.1:5000"
echo ""
echo "To run HPC in CPU mode:"
echo "  cd $HPC_DIR"
echo "  HYDRA_IFACE=lo /opt/mpich/bin/mpiexec -n 3 ./bin/load_balancer"
echo ""
echo "  (HYDRA_IFACE=lo is required in WSL to stop MPICH hanging on eth0)"
echo ""
echo "To run HPC in GPU mode (CUDA):"
echo "  cd $HPC_DIR"
echo "  USE_CUDA=1 HYDRA_IFACE=lo /opt/mpich/bin/mpiexec -n 3 ./bin/load_balancer"
echo ""

# Step 3: Start Flask dashboard (Windows CMD)
echo "[Step 3/3] Starting Flask dashboard..."
echo "  Run this in Windows CMD/PowerShell:"
echo ""
echo "  cd $DASHBOARD_DIR"
echo "  pip install -r requirements.txt  # if not already installed"
echo "  python app.py"
echo ""
echo "======================================================"
