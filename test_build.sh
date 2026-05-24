#!/bin/bash
# Simple test script to verify build

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/hpc"

echo "Cleaning..."
rm -rf obj bin
mkdir -p obj bin

echo "Building..."
make 2>&1 | tail -20

echo ""
echo "Checking binary..."
if [ -f "bin/load_balancer" ]; then
    echo "✓ SUCCESS: Binary created"
    ls -lh bin/load_balancer
else
    echo "✗ FAILED: Binary not found"
    ls -la bin/ 2>/dev/null || echo "  bin/ directory empty"
fi
