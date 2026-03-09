#!/usr/bin/env python3
"""
HPC Load Balancer Metrics Analyzer
Reads simulation_metrics.csv and generates visualizations and statistics

Dependencies: pandas, matplotlib
Install with: pip3 install pandas matplotlib
"""

import sys

try:
    import pandas as pd
    import matplotlib.pyplot as plt
except ImportError as e:
    print("=" * 60)
    print("ERROR: Required Python packages not installed")
    print("=" * 60)
    print(f"\nMissing module: {e}")
    print("\nPlease install required packages:")
    print("  pip3 install pandas matplotlib")
    print("\nOr use your package manager:")
    print("  python3 -m pip install pandas matplotlib")
    print("=" * 60)
    sys.exit(1)

def analyze_metrics(csv_file='simulation_metrics.csv'):
    """Analyze and visualize HPC load balancer metrics"""
    
    try:
        # Read CSV (skip comment lines)
        df = pd.read_csv(csv_file, comment='#')
        
        print("=" * 60)
        print("HPC LOAD BALANCER METRICS ANALYSIS")
        print("=" * 60)
        print(f"\nData loaded from: {csv_file}")
        print(f"Number of workers: {len(df)}\n")
        
        # Basic statistics
        print("SUMMARY STATISTICS")
        print("-" * 60)
        print(f"{'Metric':<30} {'Mean':<12} {'Min':<12} {'Max':<12}")
        print("-" * 60)
        
        metrics = {
            'Tasks Processed': 'tasks_processed',
            'Throughput (tasks/sec)': 'throughput',
            'Idle Time (%)': None,  # Calculated below
            'Steal Success Rate (%)': None,  # Calculated below
        }
        
        # Calculate derived metrics
        df['idle_pct'] = (df['idle_time'] / df['total_time']) * 100
        df['steal_rate'] = (df['steal_successes'] / df['steal_attempts'].replace(0, 1)) * 100
        
        metrics['Idle Time (%)'] = 'idle_pct'
        metrics['Steal Success Rate (%)'] = 'steal_rate'
        
        for label, col in metrics.items():
            if col:
                mean_val = df[col].mean()
                min_val = df[col].min()
                max_val = df[col].max()
                print(f"{label:<30} {mean_val:<12.2f} {min_val:<12.2f} {max_val:<12.2f}")
        
        # Load balance analysis
        print("\n" + "=" * 60)
        print("LOAD BALANCE ANALYSIS")
        print("=" * 60)
        total_tasks = df['tasks_processed'].sum()
        expected_per_worker = total_tasks / len(df)
        
        print(f"Total tasks processed: {total_tasks}")
        print(f"Expected per worker (perfect balance): {expected_per_worker:.0f}")
        print(f"Actual distribution:")
        
        for idx, row in df.iterrows():
            deviation = ((row['tasks_processed'] - expected_per_worker) / expected_per_worker) * 100
            print(f"  Worker {row['worker_id']}: {row['tasks_processed']:>6} tasks ({deviation:+.1f}% from ideal)")
        
        # Imbalance coefficient
        std_dev = df['tasks_processed'].std()
        cv = (std_dev / expected_per_worker) * 100  # Coefficient of variation
        print(f"\nLoad imbalance coefficient (CV): {cv:.2f}%")
        print("  (Lower is better; <10% is excellent load balance)")
        
        # Efficiency analysis
        print("\n" + "=" * 60)
        print("EFFICIENCY ANALYSIS")
        print("=" * 60)
        
        for idx, row in df.iterrows():
            compute_pct = (row['compute_time'] / row['total_time']) * 100
            idle_pct = (row['idle_time'] / row['total_time']) * 100
            print(f"Worker {row['worker_id']}: Compute={compute_pct:.2f}%, Idle={idle_pct:.2f}%")
        
        avg_idle = df['idle_pct'].mean()
        print(f"\nAverage idle time: {avg_idle:.2f}%")
        print(f"Average utilization: {100 - avg_idle:.2f}%")
        
        # Create visualizations
        print("\n" + "=" * 60)
        print("GENERATING VISUALIZATIONS")
        print("=" * 60)
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('HPC Load Balancer Performance Metrics', fontsize=16, fontweight='bold')
        
        # 1. Tasks processed
        axes[0, 0].bar(df['worker_id'], df['tasks_processed'], color='steelblue', edgecolor='black')
        axes[0, 0].axhline(expected_per_worker, color='red', linestyle='--', label='Ideal (balanced)')
        axes[0, 0].set_xlabel('Worker ID', fontweight='bold')
        axes[0, 0].set_ylabel('Tasks Processed', fontweight='bold')
        axes[0, 0].set_title('Load Distribution')
        axes[0, 0].legend()
        axes[0, 0].grid(axis='y', alpha=0.3)
        
        # 2. Throughput
        axes[0, 1].bar(df['worker_id'], df['throughput'], color='forestgreen', edgecolor='black')
        axes[0, 1].set_xlabel('Worker ID', fontweight='bold')
        axes[0, 1].set_ylabel('Tasks/Second', fontweight='bold')
        axes[0, 1].set_title('Worker Throughput')
        axes[0, 1].grid(axis='y', alpha=0.3)
        
        # 3. Idle time percentage
        axes[1, 0].bar(df['worker_id'], df['idle_pct'], color='coral', edgecolor='black')
        axes[1, 0].set_xlabel('Worker ID', fontweight='bold')
        axes[1, 0].set_ylabel('Idle Time (%)', fontweight='bold')
        axes[1, 0].set_title('Worker Idle Time')
        axes[1, 0].grid(axis='y', alpha=0.3)
        
        # 4. Steal statistics
        x = range(len(df))
        width = 0.35
        axes[1, 1].bar([i - width/2 for i in x], df['steal_attempts'], width, 
                       label='Attempts', color='lightblue', edgecolor='black')
        axes[1, 1].bar([i + width/2 for i in x], df['steal_successes'], width, 
                       label='Successes', color='darkblue', edgecolor='black')
        axes[1, 1].set_xlabel('Worker ID', fontweight='bold')
        axes[1, 1].set_ylabel('Count', fontweight='bold')
        axes[1, 1].set_title('Work Stealing Statistics')
        axes[1, 1].set_xticks(x)
        axes[1, 1].set_xticklabels(df['worker_id'])
        axes[1, 1].legend()
        axes[1, 1].grid(axis='y', alpha=0.3)
        
        plt.tight_layout()
        output_file = 'metrics_analysis.png'
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"✓ Visualization saved to: {output_file}")
        
        # Create time breakdown pie chart
        fig2, ax = plt.subplots(1, len(df), figsize=(6 * len(df), 5))
        if len(df) == 1:
            ax = [ax]
        
        for idx, row in df.iterrows():
            labels = ['Compute', 'Idle']
            sizes = [row['compute_time'], row['idle_time']]
            colors = ['#4CAF50', '#FF6B6B']
            explode = (0.05, 0)
            
            ax[idx].pie(sizes, explode=explode, labels=labels, colors=colors,
                       autopct='%1.1f%%', shadow=True, startangle=90)
            ax[idx].set_title(f'Worker {row["worker_id"]} Time Breakdown', fontweight='bold')
        
        plt.tight_layout()
        output_file2 = 'time_breakdown.png'
        plt.savefig(output_file2, dpi=150, bbox_inches='tight')
        print(f"✓ Time breakdown saved to: {output_file2}")
        
        print("\n" + "=" * 60)
        print("ANALYSIS COMPLETE")
        print("=" * 60)
        
    except FileNotFoundError:
        print(f"Error: Could not find '{csv_file}'")
        print("Make sure to run the simulation first to generate metrics.")
        sys.exit(1)
    except Exception as e:
        print(f"Error analyzing metrics: {e}")
        sys.exit(1)

if __name__ == '__main__':
    csv_file = sys.argv[1] if len(sys.argv) > 1 else 'simulation_metrics.csv'
    analyze_metrics(csv_file)
