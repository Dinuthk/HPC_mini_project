#!/usr/bin/env python3
import subprocess
import os
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
os.chdir(PROJECT_ROOT / 'hpc')

with open(PROJECT_ROOT / 'BUILD_OUTPUT.txt', 'w') as f:
    # Clean
    f.write("=== CLEANING ===\n")
    result = subprocess.run(['make', 'clean'], capture_output=True, text=True)
    f.write(result.stdout)
    if result.stderr:
        f.write("STDERR: " + result.stderr)
    
    # Build
    f.write("\n=== BUILDING ===\n")
    result = subprocess.run(['make', '-j4'], capture_output=True, text=True)
    f.write(result.stdout)
    if result.stderr:
        f.write("\nSTDERR:\n" + result.stderr)
    
    f.write("\n=== CHECKING BINARY ===\n")
    if os.path.exists('bin/load_balancer'):
        size = os.path.getsize('bin/load_balancer')
        f.write(f"✓ SUCCESS: Binary created: {size} bytes\n")
    else:
        f.write("✗ FAILED: Binary NOT found\n")
    
    # List object files
    f.write("\n=== OBJECT FILES ===\n")
    result = subprocess.run(['ls', '-lh', 'obj/'], capture_output=True, text=True)
    f.write(result.stdout)
    if result.stderr:
        f.write("STDERR: " + result.stderr)

print("Build output written to BUILD_OUTPUT.txt")
