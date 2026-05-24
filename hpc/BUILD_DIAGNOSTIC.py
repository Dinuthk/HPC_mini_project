#!/usr/bin/env python3

import subprocess
import os
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
os.chdir(PROJECT_ROOT / 'hpc')

print("=" * 60)
print("HPC BUILD DIAGNOSTIC")
print("=" * 60)

# Check prerequisites
print("\n[1] Checking prerequisites...")
result = subprocess.run(['which', 'nvcc'], capture_output=True, text=True)
if result.returncode == 0:
    print(f"✓ nvcc found: {result.stdout.strip()}")
else:
    print("✗ nvcc NOT found")
    sys.exit(1)

result = subprocess.run(['which', 'mpicc'], capture_output=True, text=True)
if result.returncode == 0:
    print(f"✓ mpicc found: {result.stdout.strip()}")
else:
    print("✗ mpicc NOT found")
    sys.exit(1)

# Clean
print("\n[2] Cleaning...")
subprocess.run(['make', 'clean'], capture_output=True)
print("✓ Cleaned")

# Try linking only first
print("\n[3] Manual linking test...")
objs = ['obj/compute.o', 'obj/config.o', 'obj/cuda_monitor.o', 'obj/dashboard.o',
        'obj/listener.o', 'obj/main.o', 'obj/master.o', 'obj/worker.o']

# Check if all object files will be created
result = subprocess.run(['make', '-n'], capture_output=True, text=True)
print("Make dry run:")
print(result.stdout[:500])

# Now actually build
print("\n[4] Building...")
result = subprocess.run(['make', '-B'], capture_output=True, text=True, timeout=120)
if "error" in result.stdout.lower() or "error" in result.stderr.lower():
    print("BUILD OUTPUT:\n", result.stdout[:1000])
    print("BUILD ERRORS:\n", result.stderr[:1000])
else:
    print("✓ Build completed")

# Check result
print("\n[5] Checking binary...")
if os.path.exists('bin/load_balancer'):
    size = os.path.getsize('bin/load_balancer')
    print(f"✓ SUCCESS: Binary created ({size} bytes)")
else:
    print("✗ FAILED: Binary not found")
    print("\nObject files:")
    subprocess.run(['ls', '-lh', 'obj/'])
    print("\nBin directory:")
    subprocess.run(['ls', '-lh', 'bin/'])
