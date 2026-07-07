#!/usr/bin/env python3
"""
Gray-box plant identification from a CLOSED-LOOP step.

The firmware has no raw-duty command, so a clean open-loop step (which
`src/host/sysid.py` needs) isn't available. Instead we identify the first-order
plant (K, tau) by minimizing the error between the firmware-faithful closed-loop
simulator (`mg513_sim_closed`, mirrored here in Python and proven bit-identical
by validate_matlab_vs_sil.py) and a captured hardware step — i.e. we fit the
plant *through the known on-chip controller*. This is exactly the model used for
the HIL overlay, so the fit optimizes the quantity we care about.

Usage:
  ~/.venvs/mg513-matlab-mcp/bin/python mcp/fit_plant_graybox.py <capture.csv> \
      [--kp 0.0015 --ki 0.01 --kd 0 --setpoint 80] [--update-params]
"""
from __future__ import annotations

import argparse
import csv as csvmod
import re
import sys
from pathlib import Path

import numpy as np
from scipy.optimize import minimize

ACTUATOR_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ACTUATOR_ROOT / "src" / "tests"))
from sil.pid_py import PIDControllerPy  # noqa: E402
from sil.motor_plant import MotorPlant  # noqa: E402

DT = 1e-3
ALPHA = 1.0 / (1.0 + 1000.0 / (2.0 * np.pi * 20.0))  # encoder LPF, fc=20 Hz
PARAMS_M = ACTUATOR_ROOT / "matlab" / "scripts" / "mg513_gmr_params.m"


def py_sim_closed(setpoint, n, kp, ki, kd, K, tau):
    """Firmware-faithful closed-loop RPM trajectory (rate limiter+PID+plant+LPF).
    Identical math to mg513_sim_closed.m / sil_runner.run_rpm_step."""
    plant = MotorPlant(K=K, tau=tau)
    pid = PIDControllerPy(kp=kp, ki=ki, kd=kd)
    rl_val = 0.0
    rpm_filt = 0.0
    traj = np.empty(n)
    for k in range(n):
        d = setpoint - rl_val
        mx = 200.0 * DT
        rl_val += max(-mx, min(mx, d))
        out = pid.update(rl_val, rpm_filt, DT)
        out = max(-1.0, min(1.0, out))
        po = plant.step(out, DT)
        rpm_filt = ALPHA * po.rpm_motor + (1.0 - ALPHA) * rpm_filt
        traj[k] = rpm_filt
    return traj


def load_capture(path: Path):
    t, rpm, out, sp = [], [], [], []
    with open(path, newline="") as f:
        for row in csvmod.DictReader(f):
            t.append(float(row["t"]) / 1000.0)
            rpm.append(float(row["rpm"]))
            out.append(float(row["out"]))
            sp.append(float(row["rpm_sp"]))
    t = np.array(t); t = t - t[0]
    return t, np.array(rpm), np.array(out), np.array(sp)


def _plant_sim(out, K, tau, dt):
    """First-order plant rpm' = (K*u - rpm)/tau, forward Euler, driven by the
    recorded duty `out`."""
    rpm = 0.0
    pred = np.empty(len(out))
    for k in range(len(out)):
        rpm += (K * out[k] - rpm) / tau * dt
        pred[k] = rpm
    return pred


def fit(csv_path: Path, kp: float, ki: float, kd: float, setpoint: float):
    """Input->output plant fit: simulate the first-order plant driven by the
    RECORDED duty (out) and fit K, tau to reproduce the recorded rpm. Properly
    identifiable (out varies during the transient), unlike a closed-loop step."""
    t_hw, rpm_hw, out_hw, sp_hw = load_capture(csv_path)
    n = len(t_hw)
    dt_hw = float(np.median(np.diff(t_hw))) if n > 1 else 0.01

    def rmse(params):
        K, tau = params
        if K <= 0 or tau <= 1e-3:
            return 1e9
        pred = _plant_sim(out_hw, K, tau, dt_hw)
        return float(np.sqrt(np.mean((rpm_hw - pred) ** 2)))

    # Seed: steady-state K ~ rpm_ss / out_ss; tau ~ 0.1 s
    out_ss = float(np.mean(out_hw[-min(50, n):]))
    rpm_ss = float(np.mean(rpm_hw[-min(50, n):]))
    K0 = rpm_ss / out_ss if out_ss > 1e-3 else 2000.0
    res = minimize(rmse, x0=[K0, 0.1], method="Nelder-Mead",
                   options={"xatol": 1e-2, "fatol": 1e-4, "maxiter": 600})
    K_fit, tau_fit = float(res.x[0]), float(res.x[1])

    # Fit quality: NRMSE fit-% (like MATLAB tfest) against the rpm signal.
    pred = _plant_sim(out_hw, K_fit, tau_fit, dt_hw)
    denom = float(np.sqrt(np.mean((rpm_hw - np.mean(rpm_hw)) ** 2)))
    fit_pct = 100.0 * (1.0 - res.fun / denom) if denom > 1e-9 else 0.0
    return {
        "K": K_fit, "tau": tau_fit, "rmse": float(res.fun), "fit_pct": fit_pct,
        "K0_seed": K0, "dt_hw": dt_hw, "n_samples": n, "csv": str(csv_path),
        "kp": kp, "ki": ki, "kd": kd, "setpoint": setpoint,
    }


def update_params_m(K: float, tau: float) -> None:
    """Write fitted K/tau into mg513_gmr_params.m (like sysid.update_sil_model)."""
    text = PARAMS_M.read_text()
    text = re.sub(r"(p\.K\s*=\s*)[0-9.eE+-]+", rf"\g<1>{K:.1f}", text, count=1)
    text = re.sub(r"(p\.tau\s*=\s*)[0-9.eE+-]+", rf"\g<1>{tau:.4f}", text, count=1)
    PARAMS_M.write_text(text)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv")
    ap.add_argument("--kp", type=float, default=0.0015)
    ap.add_argument("--ki", type=float, default=0.01)
    ap.add_argument("--kd", type=float, default=0.0)
    ap.add_argument("--setpoint", type=float, default=80.0)
    ap.add_argument("--update-params", action="store_true")
    a = ap.parse_args()
    r = fit(Path(a.csv), a.kp, a.ki, a.kd, a.setpoint)
    print(f"I/O plant fit: K={r['K']:.0f} rpm/duty  tau={r['tau']:.4f} s  "
          f"RMSE={r['rmse']:.2f} rpm  fit={r['fit_pct']:.1f}%  (n={r['n_samples']})")
    if a.update_params:
        update_params_m(r["K"], r["tau"])
        print(f"updated {PARAMS_M.name} -> p.K={r['K']:.1f}, p.tau={r['tau']:.4f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
