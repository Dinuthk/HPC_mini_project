#!/bin/bash
# Build script for HPC load balancer

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HPC_DIR="$SCRIPT_DIR/hpc"

cd "$HPC_DIR"

echo "=== HPC Load Balancer Build Script ==="
echo "Working directory: $(pwd)"
echo ""

# Step 1: Clean
echo "[1/3] Cleaning previous build..."
make clean 2>&1 | tail -5

# Step 2: Compile
echo ""
echo "[2/3] Compiling..."
make 2>&1

# Step 3: Verify
echo ""
echo "[3/3] Verifying binary..."
if [ -f "bin/load_balancer" ]; then
    echo "✓ Binary compiled successfully!"
    ls -lh bin/load_balancer
    file bin/load_balancer
else
    echo "✗ Compilation failed - binary not found"
    exit 1
fi

echo ""
echo "=== Build Complete ==="
