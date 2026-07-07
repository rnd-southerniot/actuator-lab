#!/usr/bin/env python3
"""
MG513/GMR MATLAB MCP server.

A per-actuator FastMCP server that holds ONE persistent MATLAB R2026a engine
session and (from Phase C) owns the board's serial port, so an agent can drive
the model-based-design / hardware-in-the-loop workflow programmatically.

Design (see docs plan + mcp/MCP_SERVER_CONTRACT.md):
  * Persistent `matlab.engine` — pay the startup cost once, then base-workspace
    state (loaded model, K/tau, gains) survives across tool calls.
  * Python owns the serial link (reuses the proven `serial_reader` + hw_test
    `SerialFixture` stack via `hil_bridge`); MATLAB never touches the port.
  * Model prediction is computed once per (setpoint, gains, K, tau); the live
    HIL loop overlays that fixed curve against streaming 100 Hz telemetry.

Motor-driving tools (push_gains_to_board / hil_capture / hil_overlay) require a
written motion go-ahead + 12 V and are gated behind `hil_bridge` (Phase C+).
Everything MATLAB-side (eval / sim / sysid) needs no hardware.
"""
from __future__ import annotations

import io
import os
import sys
import threading
from contextlib import redirect_stdout
from pathlib import Path
from typing import Any, Optional

from fastmcp import FastMCP

# --- Repo layout anchors -------------------------------------------------
ACTUATOR_ROOT = Path(__file__).resolve().parent.parent          # .../mg513-gmr-nucleo-f401
MATLAB_SCRIPTS = ACTUATOR_ROOT / "matlab" / "scripts"
MATLAB_MODELS = ACTUATOR_ROOT / "matlab" / "models"
HOST_DIR = ACTUATOR_ROOT / "src" / "host"                        # serial_reader.py, sysid.py
DOCS_SYSID = ACTUATOR_ROOT / "docs" / "sysid"

DEFAULT_PORT = os.environ.get("MG513_PORT", "/dev/cu.usbmodem1103")

mcp = FastMCP("mg513-matlab")

# --- Persistent MATLAB engine (lazy) -------------------------------------
_eng: Any = None
_eng_lock = threading.Lock()


def get_engine() -> Any:
    """Start (once) and return the persistent MATLAB engine, with the actuator
    matlab/ scripts + models on the MATLAB path."""
    global _eng
    with _eng_lock:
        if _eng is None:
            import matlab.engine  # deferred: only import when MATLAB is needed
            _eng = matlab.engine.start_matlab()
            for p in (MATLAB_SCRIPTS, MATLAB_MODELS):
                if p.is_dir():
                    _eng.addpath(str(p), nargout=0)
        return _eng


def _eval_capture(code: str) -> str:
    """eval MATLAB `code`, returning captured MATLAB stdout (diary-free)."""
    eng = get_engine()
    out = io.StringIO()
    err = io.StringIO()
    eng.eval(code, nargout=0, stdout=out, stderr=err)
    text = out.getvalue()
    e = err.getvalue()
    return text + (f"\n[stderr]\n{e}" if e.strip() else "")


# --- MATLAB-side tools (no hardware) -------------------------------------
@mcp.tool
def matlab_eval(code: str) -> str:
    """Evaluate MATLAB code in the persistent session and return captured
    stdout. Statements only (no local-function definitions). Base workspace
    persists across calls."""
    return _eval_capture(code)


@mcp.tool
def run_script(path: str) -> str:
    """addpath the script's folder and run a MATLAB .m script by name; return
    its stdout. `path` may be absolute or relative to matlab/scripts."""
    p = Path(path)
    if not p.is_absolute():
        p = MATLAB_SCRIPTS / p
    if not p.exists():
        return f"ERROR: script not found: {p}"
    eng = get_engine()
    eng.addpath(str(p.parent), nargout=0)
    return _eval_capture(p.stem)  # run by function/script name


@mcp.tool
def load_model(slx_path: str = "mg513_gmr_plant") -> str:
    """load_system a Simulink model (name or path; defaults to the GMR plant).
    Returns the block list."""
    name = Path(slx_path).stem
    eng = get_engine()
    return _eval_capture(
        f"load_system('{name}'); disp(getfullname(find_system('{name}','Type','Block')))"
    )


@mcp.tool
def check_toolboxes() -> dict:
    """Report which MATLAB toolboxes relevant to this workflow are licensed."""
    eng = get_engine()
    tests = {
        "simulink": "Simulink",
        "control": "Control_Toolbox",
        "sysid": "Identification_Toolbox",
        "simscape": "Simscape",
        "simulink_control_design": "Simulink_Control_Design",
        "instrument": "Instr_Control_Toolbox",
    }
    return {k: bool(eng.license("test", v)) for k, v in tests.items()}


@mcp.tool
def set_gains(kp: float, ki: float, kd: float) -> dict:
    """Push PID gains into the MATLAB base workspace (model params only; does
    NOT touch the board). Use push_gains_to_board to reach hardware."""
    eng = get_engine()
    eng.workspace["kp"] = float(kp)
    eng.workspace["ki"] = float(ki)
    eng.workspace["kd"] = float(kd)
    return {"kp": float(kp), "ki": float(ki), "kd": float(kd)}


@mcp.tool
def sim_model(
    setpoint: float,
    dur: float = 3.0,
    kp: float = 0.0015,
    ki: float = 0.01,
    kd: float = 0.0,
    K: float = 200.0,
    tau: float = 0.15,
    mode: str = "rpm",
) -> dict:
    """Run the closed-loop Simulink/first-order model once and return the
    predicted trajectory {t, rpm_pred, out_pred}. Requires the Phase-B model
    helper `mg513_sim_closed`."""
    eng = get_engine()
    try:
        res = eng.mg513_sim_closed(
            float(setpoint), float(dur), float(kp), float(ki), float(kd),
            float(K), float(tau), mode, nargout=1,
        )
    except Exception as exc:  # noqa: BLE001 - surface MATLAB errors to the agent
        return {"error": f"sim_model failed (is the Phase-B model built?): {exc}"}
    # res is a MATLAB struct -> dict of lists
    return {
        "t": list(res["t"][0]) if hasattr(res["t"], "__getitem__") else list(res["t"]),
        "rpm_pred": list(res["rpm_pred"][0]) if hasattr(res["rpm_pred"], "__getitem__") else list(res["rpm_pred"]),
        "out_pred": list(res["out_pred"][0]) if hasattr(res["out_pred"], "__getitem__") else list(res["out_pred"]),
    }


@mcp.tool
def sysid_from_csv(csv_path: str, gear_ratio: int = 30) -> dict:
    """Fit a first-order K/tau plant from a telemetry step CSV using the host's
    proven `sysid.identify_from_csv`. No hardware; no MATLAB required."""
    if str(HOST_DIR) not in sys.path:
        sys.path.insert(0, str(HOST_DIR))
    try:
        from sysid import identify_from_csv  # type: ignore
    except Exception as exc:  # noqa: BLE001
        return {"error": f"could not import host sysid: {exc}"}
    p = Path(csv_path)
    if not p.is_absolute():
        p = DOCS_SYSID / p
    if not p.exists():
        return {"error": f"csv not found: {p}"}
    try:
        r = identify_from_csv(p, gear_ratio=gear_ratio)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"sysid failed: {exc}", "csv": str(p)}
    return {
        "K": r.K, "tau": r.tau, "K_std": r.K_std, "tau_std": r.tau_std,
        "r_squared": r.r_squared, "duty": r.duty, "n_samples": r.n_samples,
        "good_fit": r.good_fit, "csv": str(p),
    }


# --- Hardware tools (Phase C+; require motion go-ahead) -------------------
def _bridge():
    """Lazy-import the serial bridge (Phase C). Kept out of import path so the
    server starts without hardware."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import hil_bridge  # type: ignore
    return hil_bridge


@mcp.tool
def estop() -> dict:
    """Immediately E-STOP the board (safe, always available once wired)."""
    try:
        return _bridge().estop(DEFAULT_PORT)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"estop unavailable (Phase C not built / no board?): {exc}"}


@mcp.tool
def clear_fault() -> dict:
    """Clear a latched FAULT/ESTOP and return to IDLE."""
    try:
        return _bridge().clear_fault(DEFAULT_PORT)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"clear_fault unavailable: {exc}"}


@mcp.tool
def push_gains_to_board(kp: float, ki: float, kd: float) -> dict:
    """Send SET_PID to the board and verify via the telemetry gain echo.
    MOTION-ADJACENT: only run with a fresh motion go-ahead. Requires Phase C."""
    try:
        return _bridge().push_gains_to_board(DEFAULT_PORT, kp, ki, kd)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"push_gains_to_board unavailable (Phase C): {exc}"}


@mcp.tool
def hil_capture(setpoint: float, dur: float = 3.0, mode: str = "rpm") -> dict:
    """Command a setpoint and stream telemetry to a CSV. SPINS THE MOTOR:
    requires a written motion go-ahead + 12 V. Requires Phase C."""
    try:
        return _bridge().hil_capture(DEFAULT_PORT, setpoint, dur, mode)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"hil_capture unavailable (Phase C / go-ahead): {exc}"}


@mcp.tool
def hil_overlay(
    setpoint: float, dur: float, kp: float, ki: float, kd: float,
    K: float, tau: float, mode: str = "rpm",
) -> dict:
    """One-shot design+validate: predict (sim_model) + capture (hil_capture) +
    overlay plot + residual RMSE. SPINS THE MOTOR. Requires Phase C/E."""
    try:
        pred = sim_model(setpoint, dur, kp, ki, kd, K, tau, mode)
        if "error" in pred:
            return pred
        return _bridge().hil_overlay(DEFAULT_PORT, setpoint, dur, kp, ki, kd, pred, mode)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"hil_overlay unavailable (Phase C/E / go-ahead): {exc}"}


if __name__ == "__main__":
    mcp.run()  # stdio transport for Claude Code
