#!/usr/bin/env python3
"""
Serial bridge for the MG513/GMR HIL loop — the ONLY owner of the board's serial
port during HIL. Reuses the proven host stack (`serial_reader` COBS/CRC +
hw_test `SerialFixture`'s atexit/SIGINT ESTOP guarantee + auto-clear-fault) so
nothing here re-implements the wire protocol.

Pre-flight: stop `src/host/server.py` and any dashboard bridge first — only one
process may hold the port.

Comms-only functions (`selftest`) need no motor power. Motion functions
(`hil_capture`, `hil_overlay`, `push_gains_to_board`) SPIN THE MOTOR and require
a written motion go-ahead + 12 V current-limited per SAFETY-CHECKLIST.md.
"""
from __future__ import annotations

import argparse
import csv
import sys
import time
from datetime import datetime
from pathlib import Path

ACTUATOR_ROOT = Path(__file__).resolve().parent.parent
HOST_DIR = ACTUATOR_ROOT / "src" / "host"
DOCS_SYSID = ACTUATOR_ROOT / "docs" / "sysid"
DEFAULT_PORT = "/dev/cu.usbmodem1103"

for _p in (str(HOST_DIR),):
    if _p not in sys.path:
        sys.path.insert(0, _p)

from serial_reader import (  # noqa: E402
    build_command, CMD_SET_MODE, CMD_SET_RPM, CMD_SET_POSITION,
    CMD_SET_PID, CMD_ESTOP, CMD_CLEAR_FAULT,
)
from hw_test.fixtures import SerialFixture  # noqa: E402

MODE_RPM = 1.0
MODE_POSITION = 2.0


def _connect(port: str) -> SerialFixture:
    sf = SerialFixture(port)
    if not sf.connect():
        raise RuntimeError(
            f"could not open/ping {port}. Is the board connected and is "
            f"server.py (or another port owner) stopped?"
        )
    return sf


def _modes(sf: SerialFixture, dur: float) -> set:
    return {p.get("mode") for p in sf.capture(dur).packets if "mode" in p}


# --- Comms-only -----------------------------------------------------------
def selftest(port: str = DEFAULT_PORT) -> dict:
    """S5: ping + telemetry + CRC sanity, then ESTOP. No motor motion."""
    sf = _connect(port)
    try:
        cap = sf.capture(1.0)
        n = len(cap.packets)
        crc_ok = n > 0 and all(("rpm" in p and "mode" in p) for p in cap.packets)
        modes = {p.get("mode") for p in cap.packets}
        moving = any(abs(p.get("rpm", 0.0)) > 5.0 for p in cap.packets)
        sf.send_raw(build_command(CMD_ESTOP))
        time.sleep(0.2)
        return {
            "ok": bool(crc_ok and not moving),
            "packets": n, "crc_ok": crc_ok, "modes": sorted(m for m in modes if m),
            "moving": moving, "port": port,
        }
    finally:
        sf.disconnect()


def estop(port: str = DEFAULT_PORT) -> dict:
    sf = _connect(port)
    try:
        sf.send_raw(build_command(CMD_ESTOP))
        time.sleep(0.2)
        return {"ok": "estop" in _modes(sf, 0.5)}
    finally:
        sf.disconnect()


def clear_fault(port: str = DEFAULT_PORT) -> dict:
    sf = _connect(port)
    try:
        sf.send_raw(build_command(CMD_CLEAR_FAULT))
        time.sleep(0.3)
        return {"ok": "idle" in _modes(sf, 0.5)}
    finally:
        sf.disconnect()


# --- Motion (require go-ahead) -------------------------------------------
def push_gains_to_board(port: str, kp: float, ki: float, kd: float) -> dict:
    """SET_PID and verify via the telemetry gain echo (retry on the state-
    transition no-op). Motion-adjacent: does not command a setpoint itself."""
    sf = _connect(port)
    try:
        sf.send_raw(build_command(CMD_CLEAR_FAULT))
        time.sleep(0.3)
        echoed = None
        for _ in range(4):
            sf.send_raw(build_command(CMD_SET_PID, p1=kp, p2=ki, p3=kd))
            time.sleep(0.4)
            pk = sf.capture(0.4).packets
            if pk:
                echoed = (pk[-1]["kp"], pk[-1]["ki"], pk[-1]["kd"])
                if (abs(echoed[0] - kp) < 1e-6 and abs(echoed[1] - ki) < 1e-6
                        and abs(echoed[2] - kd) < 1e-6):
                    break
        return {
            "ok": echoed is not None
            and abs(echoed[0] - kp) < 1e-6 and abs(echoed[1] - ki) < 1e-6,
            "echoed_kp": echoed[0] if echoed else None,
            "echoed_ki": echoed[1] if echoed else None,
            "echoed_kd": echoed[2] if echoed else None,
        }
    finally:
        sf.disconnect()


def _write_csv(packets: list[dict], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["t", "rpm", "out", "rpm_sp"])
        for p in packets:
            w.writerow([p.get("t", 0), p.get("rpm", 0), p.get("out", 0), p.get("rpm_sp", 0)])


def hil_capture(port: str, setpoint: float, dur: float = 3.0, mode: str = "rpm",
                tag: str = "") -> dict:
    """Command a setpoint and stream telemetry to a CSV. SPINS THE MOTOR."""
    sf = _connect(port)
    try:
        m = MODE_RPM if mode == "rpm" else MODE_POSITION
        want = "rpm" if mode == "rpm" else "position"
        cmd = CMD_SET_RPM if mode == "rpm" else CMD_SET_POSITION
        echo = "rpm_sp" if mode == "rpm" else "pos_sp"
        tol = max(1.0, 0.05 * abs(setpoint))
        # A single fire-and-forget SET_RPM can race the mode-entry setpoint init
        # and silently no-op (board retains the prior setpoint). Verify the
        # ramp-limited setpoint echo actually reaches the target within the
        # capture and retry the whole capture if not. Re-capturing (rather than
        # pre-settling) keeps the ramp in-frame so the overlay stays aligned.
        cap = None
        for _ in range(3):
            sf.send_raw(build_command(CMD_CLEAR_FAULT))
            time.sleep(0.3)
            for _ in range(3):  # enter mode with a verify-retry
                sf.send_raw(build_command(CMD_SET_MODE, p1=m))
                time.sleep(0.25)
                if want in _modes(sf, 0.4):
                    break
            sf.send_raw(build_command(cmd, p1=setpoint))
            cap = sf.capture(dur)
            sf.send_raw(build_command(CMD_ESTOP))
            time.sleep(0.2)
            have_echo = any(echo in p for p in cap.packets)
            reached = (not have_echo) or any(
                abs(p.get(echo, 0.0) - setpoint) <= tol for p in cap.packets)
            if reached:
                break  # setpoint applied (a mid-run stall still counts as applied)
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        suffix = f"_{tag}" if tag else ""
        csv_path = DOCS_SYSID / f"{mode}_step_{int(setpoint)}{suffix}_{stamp}.csv"
        _write_csv(cap.packets, csv_path)
        t = [p.get("t", 0) for p in cap.packets]
        rpm = [p.get("rpm", 0) for p in cap.packets]
        out = [p.get("out", 0) for p in cap.packets]
        rpm_sp = [p.get("rpm_sp", 0) for p in cap.packets]
        return {"csv_path": str(csv_path), "n": len(cap.packets),
                "t": t, "rpm": rpm, "out": out, "rpm_sp": rpm_sp}
    finally:
        sf.disconnect()


def hil_overlay(port: str, setpoint: float, dur: float, kp: float, ki: float,
                kd: float, pred: dict, mode: str = "rpm") -> dict:
    """Push gains, capture the live response, overlay the precomputed model
    prediction, save a PNG + return the residual RMSE. SPINS THE MOTOR."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np

    pg = push_gains_to_board(port, kp, ki, kd)
    cap = hil_capture(port, setpoint, dur, mode, tag="overlay")

    t_hw = np.array(cap["t"], dtype=float) / 1000.0
    t_hw = t_hw - (t_hw[0] if len(t_hw) else 0.0)
    rpm_hw = np.array(cap["rpm"], dtype=float)
    t_m = np.array(pred["t"], dtype=float)
    rpm_m = np.array(pred["rpm_pred"], dtype=float)

    rmse = None
    if len(t_hw) and len(t_m):
        rpm_m_on_hw = np.interp(t_hw, t_m, rpm_m)
        rmse = float(np.sqrt(np.mean((rpm_hw - rpm_m_on_hw) ** 2)))

    fig, ax = plt.subplots(figsize=(8, 4.5))
    ax.plot(t_m, rpm_m, label="Simulink model", lw=2)
    ax.plot(t_hw, rpm_hw, label="hardware", lw=1, alpha=0.8)
    ax.axhline(setpoint, ls="--", c="k", lw=0.6, label="setpoint")
    ax.set_xlabel("t [s]"); ax.set_ylabel("rpm (motor)")
    ax.set_title(f"HIL overlay sp={setpoint} kp={kp} ki={ki} kd={kd}"
                 + (f"  RMSE={rmse:.1f} rpm" if rmse is not None else ""))
    ax.legend(); ax.grid(True, alpha=0.3)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    png = DOCS_SYSID / f"overlay_{int(setpoint)}_{stamp}.png"
    png.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout(); fig.savefig(png, dpi=110); plt.close(fig)
    return {"csv_path": cap["csv_path"], "png_path": str(png), "rmse": rmse,
            "gains_pushed": pg}


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="MG513/GMR HIL serial bridge")
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument("--selftest", action="store_true", help="comms-only S5 (no motion)")
    args = ap.parse_args()
    if args.selftest:
        print(selftest(args.port))
    else:
        ap.error("specify --selftest (motion functions run via the MCP server "
                 "with a go-ahead)")
