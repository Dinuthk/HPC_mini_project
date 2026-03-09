#!/bin/bash
# Setup script for HPC Load Balancer analysis tools

echo "=================================================="
echo "HPC Load Balancer - Analysis Tools Setup"
echo "=================================================="

# Check if venv exists
if [ -d "venv" ]; then
    echo "✓ Virtual environment already exists"
else
    echo "Creating virtual environment..."
    python3 -m venv venv
    if [ $? -eq 0 ]; then
        echo "✓ Virtual environment created"
    else
        echo "✗ Failed to create virtual environment"
        exit 1
    fi
fi

# Activate and install packages
echo "Installing required packages..."
source venv/bin/activate

pip install --quiet pandas matplotlib

if [ $? -eq 0 ]; then
    echo "✓ Packages installed successfully"
else
    echo "✗ Failed to install packages"
    exit 1
fi

echo ""
echo "=================================================="
echo "Setup Complete!"
echo "=================================================="
echo ""
echo "To use the analysis tools:"
echo "  1. Activate the virtual environment:"
echo "     source venv/bin/activate"
echo ""
echo "  2. Run simulations:"
echo "     mpirun -np 3 ./bin/load_balancer"
echo ""
echo "  3. Analyze results:"
echo "     python analyze_metrics.py"
echo ""
echo "  4. When done, deactivate:"
echo "     deactivate"
echo ""
echo "=================================================="
