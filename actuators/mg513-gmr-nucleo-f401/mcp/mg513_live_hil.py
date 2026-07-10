#!/usr/bin/env python3
"""Live plot of the MG513/GMR closed-loop PID running ON HARDWARE.

Streams board telemetry (~100 Hz) and plots the control story in real time while
the motor tracks a setpoint:  setpoint · measured rpm · control effort (duty) ·
error.  This is LIVE HARDWARE — the real firmware PID + observer + the actual
motor — not a simulation.

*** SPINS THE MOTOR *** — requires a written motion go-ahead + 12 V, motor
secured, shaft/path clear.  Ctrl-C or closing the window sends ESTOP and
disconnects.  Uses the reliable verified-command helpers (SET_MODE/SET_RPM
echo-verify) so the commanded setpoint always lands.

Run (needs a display for the matplotlib window):
    ~/.venvs/mg513-matlab-mcp/bin/python mcp/mg513_live_hil.py --axis A --rpm 80
    ~/.venvs/mg513-matlab-mcp/bin/python mcp/mg513_live_hil.py --axis B --rpm 150 --dur 20
"""
from __future__ import annotations
import argparse
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "src" / "host"))

import matplotlib.pyplot as plt          # noqa: E402
from matplotlib.animation import FuncAnimation  # noqa: E402
from hw_test.fixtures import SerialFixture       # noqa: E402
from serial_reader import (                      # noqa: E402
    build_command, CMD_ESTOP, CMD_ESTOP_B, CMD_CLEAR_FAULT, CMD_CLEAR_FAULT_B)


def main() -> int:
    ap = argparse.ArgumentParser(description="Live hardware PID plot (MG513/GMR)")
    ap.add_argument("--port", default="/dev/cu.usbmodem1103")
    ap.add_argument("--axis", choices=["A", "B"], default="A")
    ap.add_argument("--rpm", type=float, default=80.0, help="setpoint (80-150 validated)")
    ap.add_argument("--dur", type=float, default=15.0, help="run seconds (0 = until closed)")
    ap.add_argument("--window", type=float, default=6.0, help="rolling x-axis window (s)")
    a = ap.parse_args()
    axis = 0 if a.axis == "A" else 1
    estop = CMD_ESTOP if axis == 0 else CMD_ESTOP_B
    clear = CMD_CLEAR_FAULT if axis == 0 else CMD_CLEAR_FAULT_B

    sf = SerialFixture(a.port)
    if not sf.connect():
        print("connect failed — is the board on and the port free?")
        return 1

    def safe_stop():
        try:
            sf.send_raw(build_command(CMD_ESTOP))
            sf.send_raw(build_command(CMD_ESTOP_B))
            time.sleep(0.15)
            sf.disconnect()   # also sends the fixture's safety ESTOP
        except Exception:
            pass

    # --- command the setpoint reliably --------------------------------------
    sf.send_raw(build_command(clear)); time.sleep(0.4)
    if not sf.set_mode_verified(axis, "rpm"):
        print("mode entry failed (RX drop) — retry"); safe_stop(); return 1
    sf.set_rpm_verified(axis, a.rpm)

    # --- telemetry ring + incremental deframer ------------------------------
    t0 = time.time()
    buf = bytearray()
    T, SP, RPM, OUT, ERR = [], [], [], [], []

    def pump():
        n = sf._ser.in_waiting
        if n:
            buf.extend(sf._ser.read(n))
        while 0 in buf:
            i = buf.index(0)
            frame = bytes(buf[:i]); del buf[:i + 1]
            if not frame:
                continue
            p = sf._parse_frame(frame)   # reuse fixture COBS+CRC+parse
            if p and p.get("axis", 0) == axis and "rpm" in p:
                T.append(time.time() - t0)
                SP.append(p.get("rpm_sp", 0.0)); RPM.append(p.get("rpm", 0.0))
                OUT.append(p.get("out", 0.0))
                ERR.append(p.get("rpm_sp", 0.0) - p.get("rpm", 0.0))

    # --- figure: top = speeds, bottom = duty (left) + error (right) ---------
    plt.rcParams.update({"figure.facecolor": "white", "axes.grid": True,
                         "grid.alpha": 0.3})
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9.5, 6.4), sharex=True)
    fig.suptitle(f"MG513/GMR live PID — Axis {a.axis} @ {a.rpm:.0f} rpm (HARDWARE)",
                 fontweight="bold")
    l_sp, = ax1.plot([], [], lw=1.4, ls="--", color="#888", label="setpoint")
    l_rpm, = ax1.plot([], [], lw=1.8, color="#0e8f80", label="measured rpm")
    ax1.set_ylabel("rpm"); ax1.legend(loc="lower right", fontsize=9)
    l_out, = ax2.plot([], [], lw=1.6, color="#c8551a", label="control duty")
    ax2.set_ylabel("duty", color="#c8551a"); ax2.set_xlabel("t [s]")
    ax2.tick_params(axis="y", labelcolor="#c8551a")
    ax2e = ax2.twinx()
    l_err, = ax2e.plot([], [], lw=1.2, color="#3a6ea5", alpha=0.85, label="error")
    ax2e.set_ylabel("error [rpm]", color="#3a6ea5")
    ax2e.tick_params(axis="y", labelcolor="#3a6ea5")

    def update(_):
        pump()
        if not T:
            return l_sp, l_rpm, l_out, l_err
        l_sp.set_data(T, SP); l_rpm.set_data(T, RPM)
        l_out.set_data(T, OUT); l_err.set_data(T, ERR)
        tmax = T[-1]; tmin = max(0.0, tmax - a.window)
        for ax in (ax1, ax2, ax2e):
            ax.set_xlim(tmin, tmax + 0.3)
        ax1.set_ylim(min(0, min(SP[-1], min(RPM[-300:]) if RPM else 0)) - 10,
                     max(SP[-1], max(RPM[-300:]) if RPM else a.rpm) + 15)
        w = [o for t, o in zip(T, OUT) if t >= tmin] or [0]
        ax2.set_ylim(min(w) - 0.02, max(w) + 0.02)
        e = [er for t, er in zip(T, ERR) if t >= tmin] or [0]
        m = max(5.0, max(abs(min(e)), abs(max(e))) * 1.2)
        ax2e.set_ylim(-m, m)
        if a.dur and tmax >= a.dur:
            ani.event_source.stop(); safe_stop()
        return l_sp, l_rpm, l_out, l_err

    ani = FuncAnimation(fig, update, interval=50, blit=False,
                        cache_frame_data=False)
    fig.canvas.mpl_connect("close_event", lambda _e: safe_stop())
    try:
        plt.tight_layout(rect=(0, 0, 1, 0.96))
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        safe_stop()
        print("ESTOP sent, disconnected.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
