#!/usr/bin/env python3
"""
Simple HPC Load Balancer Metrics Analyzer (No Dependencies)
Reads simulation_metrics.csv and prints statistics
"""

import sys
import csv

def analyze_metrics_simple(csv_file='simulation_metrics.csv'):
    """Analyze metrics using only standard library"""
    
    try:
        workers = []
        
        # Read CSV
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                if row['worker_id'].startswith('#'):
                    break
                workers.append({
                    'id': int(row['worker_id']),
                    'tasks': int(row['tasks_processed']),
                    'attempts': int(row['steal_attempts']),
                    'successes': int(row['steal_successes']),
                    'total_time': float(row['total_time']),
                    'compute_time': float(row['compute_time']),
                    'idle_time': float(row['idle_time']),
                    'throughput': float(row['throughput'])
                })
        
        if not workers:
            print("No worker data found in CSV")
            return
        
        print("=" * 70)
        print("HPC LOAD BALANCER METRICS ANALYSIS")
        print("=" * 70)
        print(f"\nData loaded from: {csv_file}")
        print(f"Number of workers: {len(workers)}\n")
        
        # Basic statistics
        print("WORKER PERFORMANCE")
        print("-" * 70)
        print(f"{'Worker':<8} {'Tasks':<10} {'Rate':<12} {'Idle%':<10} {'Steals':<15}")
        print("-" * 70)
        
        total_tasks = 0
        total_attempts = 0
        total_successes = 0
        
        for w in workers:
            idle_pct = (w['idle_time'] / w['total_time']) * 100 if w['total_time'] > 0 else 0
            steal_str = f"{w['successes']}/{w['attempts']}"
            
            print(f"{w['id']:<8} {w['tasks']:<10} {w['throughput']:<12.1f} {idle_pct:<10.1f} {steal_str:<15}")
            
            total_tasks += w['tasks']
            total_attempts += w['attempts']
            total_successes += w['successes']
        
        print("-" * 70)
        
        # Summary statistics
        avg_tasks = total_tasks / len(workers)
        min_tasks = min(w['tasks'] for w in workers)
        max_tasks = max(w['tasks'] for w in workers)
        
        avg_throughput = sum(w['throughput'] for w in workers) / len(workers)
        avg_idle = sum((w['idle_time'] / w['total_time']) * 100 for w in workers) / len(workers)
        
        print(f"\nTotal tasks processed: {total_tasks}")
        print(f"Average per worker: {avg_tasks:.1f}")
        print(f"Min/Max: {min_tasks} / {max_tasks}")
        print(f"Range: {max_tasks - min_tasks} ({((max_tasks - min_tasks) / avg_tasks * 100):.1f}% of average)")
        
        # Load balance coefficient
        std_dev = (sum((w['tasks'] - avg_tasks) ** 2 for w in workers) / len(workers)) ** 0.5
        cv = (std_dev / avg_tasks) * 100 if avg_tasks > 0 else 0
        
        print(f"\nLoad imbalance coefficient: {cv:.2f}%")
        if cv < 10:
            print("  ✓ Excellent load balance!")
        elif cv < 25:
            print("  ✓ Good load balance")
        elif cv < 50:
            print("  ⚠ Moderate imbalance")
        else:
            print("  ⚠ High imbalance - consider tuning parameters")
        
        print(f"\nAverage throughput: {avg_throughput:.1f} tasks/sec")
        print(f"Average idle time: {avg_idle:.2f}%")
        print(f"Average utilization: {100 - avg_idle:.2f}%")
        
        # Steal analysis
        print("\n" + "=" * 70)
        print("WORK STEALING ANALYSIS")
        print("-" * 70)
        steal_rate = (total_successes / total_attempts * 100) if total_attempts > 0 else 0
        print(f"Total steal attempts: {total_attempts}")
        print(f"Successful steals: {total_successes}")
        print(f"Success rate: {steal_rate:.1f}%")
        
        if steal_rate > 80:
            print("  ✓ Excellent steal efficiency")
        elif steal_rate > 50:
            print("  ✓ Good steal efficiency")
        elif steal_rate > 25:
            print("  ⚠ Moderate steal efficiency")
        else:
            print("  ⚠ Low steal efficiency - many idle attempts")
        
        print("\n" + "=" * 70)
        print("ANALYSIS COMPLETE")
        print("=" * 70)
        print("Tip: For visualizations, setup virtual environment:")
        print("  ./setup_analysis.sh")
        print("  source venv/bin/activate")
        print("  python analyze_metrics.py")
        print("=" * 70)
        
    except FileNotFoundError:
        print(f"Error: Could not find '{csv_file}'")
        print("Make sure to run the simulation first to generate metrics.")
        sys.exit(1)
    except Exception as e:
        print(f"Error analyzing metrics: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    csv_file = sys.argv[1] if len(sys.argv) > 1 else 'simulation_metrics.csv'
    analyze_metrics_simple(csv_file)
