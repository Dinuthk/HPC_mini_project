import os, re, subprocess, sys, threading, time


def _import_flask():
    try:
        from flask import Flask, render_template, request, jsonify
        return Flask, render_template, request, jsonify
    except ModuleNotFoundError:
        bundled_site_packages = os.path.join(
            os.path.dirname(__file__),
            'venv',
            'lib',
            'python3.12',
            'site-packages',
        )
        if os.path.isdir(bundled_site_packages) and bundled_site_packages not in sys.path:
            sys.path.insert(0, bundled_site_packages)
            from flask import Flask, render_template, request, jsonify
            return Flask, render_template, request, jsonify
        raise


Flask, render_template, request, jsonify = _import_flask()

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
            # Flask runs inside WSL here, so execute commands directly in bash.
            exec_args = ['bash', '-lc', cmd]

            self.process = subprocess.Popen(
                exec_args,
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

runners = {
    'hpc': SimRunner('HPC', os.path.join(PROJECT_ROOT, 'hpc'),
                     'make clean && make', '/opt/mpich2/bin/mpiexec -n 3 ./bin/load_balancer'),
    'hpc_gpu': SimRunner('HPC (GPU)', os.path.join(PROJECT_ROOT, 'hpc'),
                         'make clean && make', 'USE_CUDA=1 /opt/mpich2/bin/mpiexec -n 3 ./bin/load_balancer'),
    'no_balancer': SimRunner('No Balancer', os.path.join(PROJECT_ROOT, 'no_balancer'),
                             'make clean && make', './bin/no_balancer')
}

# --- Parameter Management ---
def read_params(mode):
    p = {}
    if mode.startswith('hpc'):
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
    if mode.startswith('hpc'):
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
        run_override = f'/opt/mpich2/bin/mpiexec -n {np_val} ./bin/load_balancer'
    ok = runners[mode].run(run_override)
    return jsonify({'ok': ok})

@app.route('/api/<mode>/stop', methods=['POST'])
def stop_sim(mode):
    return jsonify({'ok': runners[mode].stop()})

@app.route('/api/<mode>/status')
def status(mode):
    from_idx = int(request.args.get('from', 0))
    return jsonify(runners[mode].get_state(from_idx))

if __name__ == '__main__':
    print("\n" + "="*60)
    print("  HPC SIMULATION DASHBOARD")
    print("  Open: http://127.0.0.1:5000")
    print("="*60 + "\n")
    app.run(debug=False, host='0.0.0.0', port=5000)
