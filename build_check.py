#!/usr/bin/env python3
import subprocess
import os
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
os.chdir(PROJECT_ROOT / 'hpc')

# Clean
print("=== CLEANING ===")
result = subprocess.run(['make', 'clean'], capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print("STDERR:", result.stderr)

# Build
print("\n=== BUILDING ===")
result = subprocess.run(['make'], capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print("STDERR:", result.stderr)

print("\n=== CHECKING BINARY ===")
if os.path.exists('bin/load_balancer'):
    size = os.path.getsize('bin/load_balancer')
    print(f"✓ Binary created: {size} bytes")
else:
    print("✗ Binary NOT found")

# List object files
print("\n=== OBJECT FILES ===")
result = subprocess.run(['ls', '-lh', 'obj/'], capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print("STDERR:", result.stderr)
