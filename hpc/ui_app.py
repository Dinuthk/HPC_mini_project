import subprocess
from pathlib import Path

import pandas as pd
import streamlit as st

st.set_page_config(page_title="HPC Load Balancer UI", layout="wide")

PROJECT_DIR = Path(__file__).resolve().parent
CSV_PATH = PROJECT_DIR / "simulation_metrics.csv"


def run_command(command: list[str]) -> tuple[int, str]:
    result = subprocess.run(
        command,
        cwd=PROJECT_DIR,
        capture_output=True,
        text=True,
    )
    output = (result.stdout or "") + ("\n" + result.stderr if result.stderr else "")
    if not output.strip():
        output = "(no stdout/stderr output)"
    command_str = " ".join(command)
    formatted = f"$ {command_str}\n[exit code: {result.returncode}]\n\n{output}"
    return result.returncode, formatted


def load_metrics() -> pd.DataFrame | None:
    if not CSV_PATH.exists():
        return None
    try:
        return pd.read_csv(CSV_PATH, comment="#")
    except Exception:
        return None


st.title("HPC Distributed Load Balancer Dashboard")
st.caption("Build, run, and analyze your MPI + OpenMP simulation from a simple UI.")

if "build_output" not in st.session_state:
    st.session_state.build_output = ""
if "run_output" not in st.session_state:
    st.session_state.run_output = ""
if "analysis_output" not in st.session_state:
    st.session_state.analysis_output = ""

with st.sidebar:
    st.header("Simulation Config")
    ranks = st.number_input("MPI ranks (total processes)", min_value=3, max_value=64, value=3, step=1)
    total_tasks = st.number_input("Total tasks", min_value=1000, max_value=2_000_000, value=90_000, step=1000)
    imbalance_ratio = st.number_input("Imbalance ratio", min_value=1.0, max_value=20.0, value=8.0, step=0.5)
    batch_size = st.number_input("Batch size", min_value=64, max_value=16384, value=1024, step=64)
    low_watermark = st.number_input("Low watermark", min_value=10, max_value=100_000, value=1000, step=10)
    sim_seconds = st.number_input("Simulation time (seconds)", min_value=1.0, max_value=120.0, value=10.0, step=1.0)

col1, col2, col3 = st.columns(3)

with col1:
    if st.button("Build Project", width="stretch"):
        code, out = run_command(["make", "clean"])
        code2, out2 = run_command(["make"])
        st.session_state.build_output = out + "\n\n" + out2
        if code == 0 and code2 == 0:
            st.success("Build completed successfully")
        else:
            st.error("Build failed")

with col2:
    if st.button("Run Simulation", width="stretch"):
        command = [
            "mpirun",
            "-np",
            str(int(ranks)),
            "./bin/load_balancer",
            "-t",
            str(int(total_tasks)),
            "-r",
            str(float(imbalance_ratio)),
            "-b",
            str(int(batch_size)),
            "-w",
            str(int(low_watermark)),
            "-s",
            str(float(sim_seconds)),
        ]
        code, out = run_command(command)
        st.session_state.run_output = out
        if code == 0:
            st.success("Simulation completed")
        else:
            st.error("Simulation failed")

with col3:
    if st.button("Run Quick Analysis", width="stretch"):
        code, out = run_command(["python3", "analyze_metrics_simple.py"])
        st.session_state.analysis_output = out
        if code == 0:
            st.success("Analysis completed")
        else:
            st.error("Analysis failed")

left, right = st.columns([1, 1])

with left:
    st.subheader("Build Output")
    st.code(st.session_state.build_output or "No build output yet. Click 'Build Project'.", language="bash")

    st.subheader("Run Output")
    st.code(st.session_state.run_output or "No run output yet. Click 'Run Simulation'.", language="bash")

with right:
    st.subheader("Analysis Output")
    st.code(st.session_state.analysis_output or "No analysis output yet. Click 'Run Quick Analysis'.", language="bash")

    st.subheader("Metrics Preview")
    metrics = load_metrics()
    if metrics is None:
        st.info("No `simulation_metrics.csv` found yet. Run a simulation first.")
    else:
        st.dataframe(metrics, width="stretch")

        chart_cols = st.columns(2)
        with chart_cols[0]:
            st.markdown("**Tasks Processed**")
            st.bar_chart(metrics.set_index("worker_id")["tasks_processed"])

        with chart_cols[1]:
            st.markdown("**Throughput**")
            st.bar_chart(metrics.set_index("worker_id")["throughput"])

        if {"idle_time", "total_time"}.issubset(metrics.columns):
            metrics_plot = metrics.copy()
            metrics_plot["idle_pct"] = (metrics_plot["idle_time"] / metrics_plot["total_time"]) * 100.0
            st.markdown("**Idle Time (%)**")
            st.bar_chart(metrics_plot.set_index("worker_id")["idle_pct"])
