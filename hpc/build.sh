#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
echo "=== BUILD START ===" > /tmp/build_output.txt
echo "Working directory: $(pwd)" >> /tmp/build_output.txt
echo "CUDA check:" >> /tmp/build_output.txt
which nvcc >> /tmp/build_output.txt 2>&1
nvcc --version >> /tmp/build_output.txt 2>&1
echo "" >> /tmp/build_output.txt
echo "=== CLEANING ===" >> /tmp/build_output.txt
make clean >> /tmp/build_output.txt 2>&1
echo "" >> /tmp/build_output.txt
echo "=== LISTING SOURCES ===" >> /tmp/build_output.txt
ls -la src/ >> /tmp/build_output.txt 2>&1
echo "" >> /tmp/build_output.txt
echo "=== BUILDING ===" >> /tmp/build_output.txt
make >> /tmp/build_output.txt 2>&1
BUILD_EXIT=$?
echo "" >> /tmp/build_output.txt
echo "=== BUILD EXIT CODE: $BUILD_EXIT ===" >> /tmp/build_output.txt
echo "" >> /tmp/build_output.txt
echo "=== CHECKING RESULT ===" >> /tmp/build_output.txt
ls -lh obj/ >> /tmp/build_output.txt 2>&1
echo "" >> /tmp/build_output.txt
ls -lh bin/ >> /tmp/build_output.txt 2>&1
echo "" >> /tmp/build_output.txt
echo "=== BUILD COMPLETE ===" >> /tmp/build_output.txt
cat /tmp/build_output.txt
