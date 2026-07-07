#!/usr/bin/env python3
"""
S2/S3 gate — prove the MATLAB model is faithful to the firmware SIL ground truth.

  S2  PID equivalence : feed an identical (setpoint, measurement) sequence
      (including saturation excursions that exercise the anti-windup) to both
      mg513_pid_fcn (MATLAB) and PIDControllerPy (pid_py.py); assert max|Δu|<1e-6.
  S3  Closed-loop     : run mg513_sim_closed (MATLAB) and the sil_runner RPM loop
      (Python) with identical K/tau/gains/setpoint; assert trajectory RMSE<0.5 RPM.

Run:  ~/.venvs/mg513-matlab-mcp/bin/python mcp/validate_matlab_vs_sil.py
Needs the MATLAB engine venv; no hardware.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

import numpy as np

ACTUATOR_ROOT = Path(__file__).resolve().parent.parent
SIL_DIR = ACTUATOR_ROOT / "src" / "tests"
MATLAB_SCRIPTS = ACTUATOR_ROOT / "matlab" / "scripts"

sys.path.insert(0, str(SIL_DIR))
from sil.pid_py import PIDControllerPy  # noqa: E402
from sil.motor_plant import MotorPlant  # noqa: E402

DT = 1e-3
ALPHA = 1.0 / (1.0 + 1000.0 / (2.0 * math.pi * 20.0))  # encoder LPF, fc=20 Hz


def py_pid_run(sp, meas, kp, ki, kd):
    pid = PIDControllerPy(kp=kp, ki=ki, kd=kd)
    return [pid.update(float(s), float(m), DT) for s, m in zip(sp, meas)]


def py_sim_closed(setpoint, dur, kp, ki, kd, K, tau):
    """Mirror sil_runner.run_rpm_step, capturing the rpm_filt trajectory."""
    plant = MotorPlant(K=K, tau=tau)
    pid = PIDControllerPy(kp=kp, ki=ki, kd=kd)
    rl_val = 0.0
    rpm_filt = 0.0
    n = round(dur / DT)
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


def main() -> int:
    import matlab.engine
    eng = matlab.engine.start_matlab()
    eng.addpath(str(MATLAB_SCRIPTS), nargout=0)

    ok = True

    # ---- S2: PID equivalence, with anti-windup excursions ----------------
    rng = np.random.default_rng(0)
    n = 4000
    # Setpoint: steps that push the PID into saturation (big) then back.
    sp = np.concatenate([
        np.full(1000, 150.0), np.full(1000, -150.0),
        np.full(1000, 5.0), np.full(1000, 80.0),
    ])
    # Measurement: a lagging noisy follower so error changes sign (exercises AW).
    meas = np.zeros(n)
    for i in range(1, n):
        meas[i] = 0.98 * meas[i - 1] + 0.02 * sp[i] + rng.normal(0, 1.0)
    kp, ki, kd = 0.0015, 0.01, 0.0

    u_py = np.array(py_pid_run(sp, meas, kp, ki, kd))
    u_ml = np.array(eng.mg513_pid_run(
        matlab.double(sp.tolist()), matlab.double(meas.tolist()),
        float(DT), float(kp), float(ki), float(kd), nargout=1,
    )).ravel()
    d2 = float(np.max(np.abs(u_py - u_ml)))
    s2_pass = d2 < 1e-6
    ok &= s2_pass
    print(f"S2 PID equivalence:      max|du| = {d2:.3e}   -> {'PASS' if s2_pass else 'FAIL'}")

    # Also with a nonzero kd (exercise the filtered-derivative recurrence)
    u_py2 = np.array(py_pid_run(sp, meas, 0.002, 0.02, 0.0005))
    u_ml2 = np.array(eng.mg513_pid_run(
        matlab.double(sp.tolist()), matlab.double(meas.tolist()),
        float(DT), 0.002, 0.02, 0.0005, nargout=1,
    )).ravel()
    d2b = float(np.max(np.abs(u_py2 - u_ml2)))
    s2b_pass = d2b < 1e-6
    ok &= s2b_pass
    print(f"S2 PID (with kd):        max|du| = {d2b:.3e}   -> {'PASS' if s2b_pass else 'FAIL'}")

    # ---- S3: closed-loop equivalence vs SIL ------------------------------
    for (setpoint, K, tau, kp, ki, kd) in [
        (80.0, 200.0, 0.15, 0.0015, 0.01, 0.0),
        (150.0, 200.0, 0.15, 0.0015, 0.01, 0.0),
        (100.0, 180.0, 0.12, 0.002, 0.02, 0.0),
    ]:
        traj_py = py_sim_closed(setpoint, 3.0, kp, ki, kd, K, tau)
        res = eng.mg513_sim_closed(
            float(setpoint), 3.0, float(kp), float(ki), float(kd),
            float(K), float(tau), "rpm", nargout=1,
        )
        traj_ml = np.array(res["rpm_pred"]).ravel()
        rmse = float(np.sqrt(np.mean((traj_py - traj_ml) ** 2)))
        s3_pass = rmse < 0.5
        ok &= s3_pass
        print(f"S3 closed-loop sp={setpoint:5.0f} K={K:.0f} tau={tau:.2f}: "
              f"RMSE = {rmse:.3e} RPM   -> {'PASS' if s3_pass else 'FAIL'}")

    eng.quit()
    print("\nOVERALL:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
