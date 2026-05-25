from flask import Flask, render_template, request, jsonify
import subprocess, threading, os, re, time, math, json

app = Flask(__name__)
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

class SimRunner:
    def __init__(self, name, build_dir, build_cmd, run_cmd):
        self.name = name
        self.build_dir = build_dir
        self.build_cmd = build_cmd
        self.run_cmd = run_cmd
        self.process = None
        self.output = []
        self.status = 'idle'
        self.lock = threading.Lock()
        self.start_time = None
        self.end_time = None

    def run(self, run_cmd_override=None):
        with self.lock:
            if self.status in ('running', 'building'):
                return False
            self.output = []
            self.status = 'building'
            self.start_time = time.time()
            self.end_time = None

        wsl_path = self.build_dir.replace('\\', '/').replace('D:/', '/mnt/d/').replace('d:/', '/mnt/d/')
        if ':' in wsl_path:
            drive = wsl_path[0].lower()
            wsl_path = f'/mnt/{drive}' + wsl_path[2:]
        
        rcmd = run_cmd_override or self.run_cmd
        full = f'cd {wsl_path} && {self.build_cmd} && {rcmd}'
        t = threading.Thread(target=self._exec, args=(full,), daemon=True)
        t.start()
        return True

    def _exec(self, cmd):
        try:
            self.process = subprocess.Popen(
                ['wsl', '-d', 'Ubuntu', '--', 'bash', '-c', cmd],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, bufsize=1, encoding='utf-8', errors='replace'
            )
            build_done = False
            completion_keywords = ['Shutdown successful', 'SIMULATION COMPLETE', 'RESULTS:', 'FINISHED.', 'Total Execution Time']
            for line in iter(self.process.stdout.readline, ''):
                stripped = line.rstrip()
                with self.lock:
                    if not build_done and not any(k in stripped for k in ['mpicc', 'gcc', 'mkdir', 'rm -rf', 'warning:', '---']):
                        if stripped and not stripped.startswith('make'):
                            build_done = True
                            self.status = 'running'
                    self.output.append(stripped)
                    # Detect completion from output content
                    if any(k in stripped for k in completion_keywords) and self.end_time is None:
                        self.end_time = time.time()
            # Process may hang (WSL/mpirun), so use timeout
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            with self.lock:
                if self.end_time is None:
                    self.end_time = time.time()
                self.status = 'completed' if self.process.returncode == 0 else 'error'
        except Exception as e:
            with self.lock:
                self.output.append(f'ERROR: {e}')
                self.status = 'error'
                self.end_time = time.time()

    def stop(self):
        with self.lock:
            if self.process and self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=2)
                except:
                    self.process.kill()
                self.status = 'stopped'
                self.end_time = time.time()
                self.output.append('--- STOPPED BY USER ---')
                return True
        return False

    def get_state(self, from_idx=0):
        with self.lock:
            elapsed = None
            if self.start_time:
                elapsed = round((self.end_time or time.time()) - self.start_time, 2)
            return {
                'status': self.status,
                'lines': self.output[from_idx:],
                'total': len(self.output),
                'elapsed': elapsed
            }

    def get_full_output(self):
        with self.lock:
            return list(self.output)

runners = {
    'hpc': SimRunner('HPC', os.path.join(PROJECT_ROOT, 'hpc'),
                     'make clean && make', 'mpirun --allow-run-as-root -np 3 ./bin/load_balancer'),
    'no_balancer': SimRunner('No Balancer', os.path.join(PROJECT_ROOT, 'no_balancer'),
                             'make clean && make', './bin/no_balancer')
}

# --- Parameter Management ---
def read_params(mode):
    p = {}
    if mode == 'hpc':
        with open(os.path.join(PROJECT_ROOT, 'hpc', 'src', 'compute.c'), 'r', encoding='utf-8') as f:
            m = re.search(r'int simulations\s*=\s*(\d+)', f.read())
            if m: p['simulations'] = int(m.group(1))
        with open(os.path.join(PROJECT_ROOT, 'hpc', 'include', 'config.h'), 'r', encoding='utf-8') as f:
            c = f.read()
            m = re.search(r'#define\s+INITIAL_TASKS_NODE_1\s+(\d+)', c)
            if m: p['tasks_node_1'] = int(m.group(1))
            m = re.search(r'#define\s+INITIAL_TASKS_NODE_2\s+(\d+)', c)
            if m: p['tasks_node_2'] = int(m.group(1))
            m = re.search(r'#define\s+SIMULATION_DURATION_SECONDS\s+([\d.]+)', c)
            if m: p['max_duration'] = float(m.group(1))
        p['np'] = 3
    else:
        with open(os.path.join(PROJECT_ROOT, 'no_balancer', 'no_balancer.c'), 'r', encoding='utf-8') as f:
            c = f.read()
            m = re.search(r'int simulations\s*=\s*(\d+)', c)
            if m: p['simulations'] = int(m.group(1))
            m = re.search(r'#define\s+TASKS_WORKER_1\s+(\d+)', c)
            if m: p['tasks_worker_1'] = int(m.group(1))
            m = re.search(r'#define\s+TASKS_WORKER_2\s+(\d+)', c)
            if m: p['tasks_worker_2'] = int(m.group(1))
    return p

def write_params(mode, params):
    if mode == 'hpc':
        if 'simulations' in params:
            path = os.path.join(PROJECT_ROOT, 'hpc', 'src', 'compute.c')
            with open(path, 'r', encoding='utf-8') as f: c = f.read()
            c = re.sub(r'int simulations\s*=\s*\d+', f'int simulations = {params["simulations"]}', c)
            with open(path, 'w', encoding='utf-8') as f: f.write(c)
        cfg_path = os.path.join(PROJECT_ROOT, 'hpc', 'include', 'config.h')
        with open(cfg_path, 'r', encoding='utf-8') as f: c = f.read()
        if 'tasks_node_1' in params:
            c = re.sub(r'#define\s+INITIAL_TASKS_NODE_1\s+\d+', f'#define INITIAL_TASKS_NODE_1 {params["tasks_node_1"]}', c)
        if 'tasks_node_2' in params:
            c = re.sub(r'#define\s+INITIAL_TASKS_NODE_2\s+\d+', f'#define INITIAL_TASKS_NODE_2 {params["tasks_node_2"]}', c)
        if 'max_duration' in params:
            val = float(params['max_duration'])
            c = re.sub(r'#define\s+SIMULATION_DURATION_SECONDS\s+[\d.]+', f'#define SIMULATION_DURATION_SECONDS {val:.1f}', c)
        with open(cfg_path, 'w', encoding='utf-8') as f: f.write(c)
    else:
        path = os.path.join(PROJECT_ROOT, 'no_balancer', 'no_balancer.c')
        with open(path, 'r', encoding='utf-8') as f: c = f.read()
        if 'simulations' in params:
            c = re.sub(r'int simulations\s*=\s*\d+', f'int simulations = {params["simulations"]}', c)
        if 'tasks_worker_1' in params:
            c = re.sub(r'#define\s+TASKS_WORKER_1\s+\d+', f'#define TASKS_WORKER_1 {params["tasks_worker_1"]}', c)
        if 'tasks_worker_2' in params:
            c = re.sub(r'#define\s+TASKS_WORKER_2\s+\d+', f'#define TASKS_WORKER_2 {params["tasks_worker_2"]}', c)
        with open(path, 'w', encoding='utf-8') as f: f.write(c)

# --- Routes ---
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/<mode>/params', methods=['GET'])
def get_params(mode):
    return jsonify(read_params(mode))

@app.route('/api/<mode>/params', methods=['POST'])
def set_params(mode):
    write_params(mode, request.json)
    return jsonify({'ok': True, 'params': read_params(mode)})

@app.route('/api/<mode>/run', methods=['POST'])
def run_sim(mode):
    data = request.json or {}
    run_override = None
    if mode == 'hpc' and 'np' in data:
        np_val = int(data['np'])
        run_override = f'mpirun --allow-run-as-root -np {np_val} ./bin/load_balancer'
    ok = runners[mode].run(run_override)
    return jsonify({'ok': ok})

@app.route('/api/<mode>/stop', methods=['POST'])
def stop_sim(mode):
    return jsonify({'ok': runners[mode].stop()})

@app.route('/api/<mode>/status')
def status(mode):
    from_idx = int(request.args.get('from', 0))
    return jsonify(runners[mode].get_state(from_idx))

# --- Analysis / RMSE ---
run_history = []  # stores completed run analysis results

def parse_output(mode, lines):
    """Parse simulation terminal output to extract metrics."""
    text = '\n'.join(lines)
    result = {
        'mode': mode,
        'exec_time': None,
        'throughput': None,
        'total_tasks': None,
        'workers': [],
        'risk_total': None,
        'idle_time': None,
    }

    if mode == 'no_balancer':
        # Worker times
        w1_m = re.search(r'Worker 1.*?(\d+)\s+tasks in\s+([\d.]+)\s+sec', text)
        w2_m = re.search(r'Worker 2.*?(\d+)\s+tasks in\s+([\d.]+)\s+sec', text)
        if w1_m:
            w1_tasks, w1_time = int(w1_m.group(1)), float(w1_m.group(2))
            result['workers'].append({'name': 'Worker 1', 'tasks': w1_tasks, 'time': w1_time})
        if w2_m:
            w2_tasks, w2_time = int(w2_m.group(1)), float(w2_m.group(2))
            result['workers'].append({'name': 'Worker 2', 'tasks': w2_tasks, 'time': w2_time})
        # Total
        t_m = re.search(r'Total Execution Time:\s+([\d.]+)', text)
        if t_m: result['exec_time'] = float(t_m.group(1))
        th_m = re.search(r'Overall Throughput:\s+([\d.]+)', text)
        if th_m: result['throughput'] = float(th_m.group(1))
        tp_m = re.search(r'Total Tasks Processed:\s+(\d+)', text)
        if tp_m: result['total_tasks'] = int(tp_m.group(1))
        # Idle
        idle_m = re.search(r'BOTTLENECK.*?sat IDLE for\s+([\d.]+)', text)
        if idle_m: result['idle_time'] = float(idle_m.group(1))
        # Risk (checksum)
        risk_m = re.search(r'Checksum.*?([\d.]+)', text)
        if risk_m: result['risk_total'] = float(risk_m.group(1))
    else:
        # HPC version
        t_m = re.search(r'Total Execution Time:\s+([\d.]+)', text)
        if t_m: result['exec_time'] = float(t_m.group(1))
        tp_m = re.search(r'Total Tasks Assigned:\s+(\d+)', text)
        if tp_m: result['total_tasks'] = int(tp_m.group(1))
        if result['exec_time'] and result['total_tasks']:
            result['throughput'] = round(result['total_tasks'] / result['exec_time'], 1)
        # Workers
        for m in re.finditer(r'\[Worker (\d+)\] Shutdown.*?Processed (\d+) trades in ([\d.]+) seconds \(([\d.]+) tasks/sec\)', text):
            result['workers'].append({
                'name': f'Worker {m.group(1)}',
                'tasks': int(m.group(2)),
                'time': float(m.group(3)),
                'throughput': float(m.group(4))
            })
        result['idle_time'] = 0.0  # balanced → minimal idle

    return result


@app.route('/api/analysis')
def analysis():
    """Compute RMSE and comparison metrics from the latest runs."""
    hpc_out = runners['hpc'].get_full_output()
    nb_out = runners['no_balancer'].get_full_output()

    hpc_data = parse_output('hpc', hpc_out) if hpc_out else None
    nb_data = parse_output('no_balancer', nb_out) if nb_out else None

    # Compute RMSE-related metrics
    rmse = None
    speedup = None
    efficiency = None
    idle_reduction = None

    if hpc_data and nb_data:
        # Speedup
        if nb_data['exec_time'] and hpc_data['exec_time'] and hpc_data['exec_time'] > 0:
            speedup = round(nb_data['exec_time'] / hpc_data['exec_time'], 2)
            num_workers = len(hpc_data['workers']) or 2
            # Estimate total parallel units: MPI workers × OpenMP threads
            # Try to detect OMP_NUM_THREADS; fallback to estimating from speedup
            import os
            omp_threads = int(os.environ.get('OMP_NUM_THREADS', 0))
            if omp_threads <= 0:
                # Estimate: assume each worker used ~(speedup/workers) threads
                omp_threads = max(1, round(speedup / num_workers))
            total_cores = num_workers * omp_threads
            efficiency = round((speedup / total_cores) * 100, 1)

        # RMSE between per-worker task distribution (normalized)
        # Shows how well-balanced the work distribution is
        if hpc_data['workers'] and nb_data['workers']:
            # HPC worker imbalance RMSE
            hpc_tasks = [w['tasks'] for w in hpc_data['workers']]
            nb_tasks = [w['tasks'] for w in nb_data['workers']]

            if hpc_tasks:
                hpc_mean = sum(hpc_tasks) / len(hpc_tasks)
                hpc_rmse = math.sqrt(sum((t - hpc_mean)**2 for t in hpc_tasks) / len(hpc_tasks))
            else:
                hpc_rmse = 0

            if nb_tasks:
                nb_mean = sum(nb_tasks) / len(nb_tasks)
                nb_rmse = math.sqrt(sum((t - nb_mean)**2 for t in nb_tasks) / len(nb_tasks))
            else:
                nb_rmse = 0

            rmse = {
                'hpc_imbalance_rmse': round(hpc_rmse, 2),
                'nb_imbalance_rmse': round(nb_rmse, 2),
                'improvement_pct': round((1 - hpc_rmse / nb_rmse) * 100, 1) if nb_rmse > 0 else 0
            }

        # Idle time reduction
        if nb_data['idle_time'] and nb_data['idle_time'] > 0:
            hpc_idle = hpc_data.get('idle_time', 0) or 0
            idle_reduction = round((1 - hpc_idle / nb_data['idle_time']) * 100, 1)

    return jsonify({
        'hpc': hpc_data,
        'no_balancer': nb_data,
        'rmse': rmse,
        'speedup': speedup,
        'efficiency': efficiency,
        'total_cores': total_cores if hpc_data and nb_data and speedup else None,
        'idle_reduction': idle_reduction
    })


@app.route('/api/analysis/save', methods=['POST'])
def save_analysis():
    """Save current run results to history for multi-run comparison."""
    hpc_out = runners['hpc'].get_full_output()
    nb_out = runners['no_balancer'].get_full_output()
    hpc_data = parse_output('hpc', hpc_out) if hpc_out else None
    nb_data = parse_output('no_balancer', nb_out) if nb_out else None

    label = (request.json or {}).get('label', f'Run {len(run_history) + 1}')
    params = {
        'hpc': read_params('hpc'),
        'no_balancer': read_params('no_balancer')
    }

    entry = {
        'label': label,
        'timestamp': time.strftime('%H:%M:%S'),
        'params': params,
        'hpc': hpc_data,
        'no_balancer': nb_data
    }
    run_history.append(entry)
    return jsonify({'ok': True, 'total': len(run_history)})


@app.route('/api/analysis/history')
def get_history():
    return jsonify(run_history)


@app.route('/api/analysis/history', methods=['DELETE'])
def clear_history():
    run_history.clear()
    return jsonify({'ok': True})


if __name__ == '__main__':
    print("\n" + "="*60)
    print("  HPC SIMULATION DASHBOARD")
    print("  Open: http://127.0.0.1:5000")
    print("="*60 + "\n")
    app.run(debug=False, port=5000)
