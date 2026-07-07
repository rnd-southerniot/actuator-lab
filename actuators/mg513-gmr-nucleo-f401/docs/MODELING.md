# MODELING — MG513/GMR + DBH-12V + NUCLEO-F401RE · MBSD / live HIL

Model-based system design workflow: a MATLAB/Simulink plant + firmware-faithful PID, driven
through a **MATLAB MCP server**, with a **live hardware-in-the-loop (HIL)** design/validate loop
(push gains → command setpoints → overlay live hardware vs the model). Built 2026-07-07 on the
commissioned Axis-A rig (baked gains kp=0.0015/ki=0.01/kd=0). Sibling method reference:
[../../mg513p30-hall-motor/docs/MODELING.md](../../mg513p30-hall-motor/docs/MODELING.md).

## What was built (all validated)
| Component | File | Status |
|---|---|---|
| MATLAB MCP server (persistent engine, 12 tools) | [`../mcp/matlab_mcp_server.py`](../mcp/matlab_mcp_server.py) | ✅ registered (`.mcp.json`) |
| Firmware-faithful PID (ISA, b=1/c=0/N=10, back-calc AW) | [`../matlab/scripts/mg513_pid_fcn.m`](../matlab/scripts/mg513_pid_fcn.m) | ✅ **bit-exact** vs `pid_py.py` |
| Closed-loop predictor (rate-limiter+PID+plant+enc-LPF) | [`../matlab/scripts/mg513_sim_closed.m`](../matlab/scripts/mg513_sim_closed.m) | ✅ **bit-exact** vs SIL |
| Params (K,τ + control consts → base ws) | [`../matlab/scripts/mg513_gmr_params.m`](../matlab/scripts/mg513_gmr_params.m) | ✅ (templated on QUBE) |
| Simulink design model (tunable PID + plant) | [`../matlab/models/mg513_gmr_plant.slx`](../matlab/models/mg513_gmr_plant.slx) | ✅ builds + simulates |
| Serial HIL bridge (COBS/CRC + ESTOP guarantee) | [`../mcp/hil_bridge.py`](../mcp/hil_bridge.py) | ✅ comms-verified |
| Plant fit + overlay | [`../mcp/fit_plant_graybox.py`](../mcp/fit_plant_graybox.py), `hil_overlay` | ✅ pipeline runs on HW |

- **Faithfulness gate (S2/S3):** the MATLAB PID and the closed-loop predictor are **numerically
  identical** (max|Δu|=0, trajectory RMSE=0) to the firmware SIL ground truth
  (`src/tests/sil/pid_py.py` + `motor_plant.py`) — both IEEE-754 double, same op order. So the
  model represents the *as-flashed* firmware exactly. Harness:
  [`../mcp/validate_matlab_vs_sil.py`](../mcp/validate_matlab_vs_sil.py).
- **MATLAB R2026a**, licensed: Simulink, Control System, **System Identification**, Simscape
  (+ Simulink Control Design, Motor Control Blockset). Engine via a python3.12 venv
  (`~/.venvs/mg513-matlab-mcp`); the 3.14 system Python can't host the engine.

## ⚠️ Plant identification — key finding (data-limited on current firmware)
A first-order duty→RPM plant `G(s)=K/(τs+1)` could **not** be cleanly identified on the bench with
the current firmware. Three attempts, all on live captures (`docs/sysid/`):

1. **`sysid.py` (open-loop first-order fit) on a closed-loop step** → failed (K-guess out of bounds;
   the fit expects an open-loop step, not a controller-shaped response).
2. **Gray-box closed-loop fit** (match `mg513_sim_closed` to hardware) → diverged (K→1e16). With an
   **integrator**, the closed-loop steady state reaches setpoint for *any* K → the plant is
   unidentifiable from a closed-loop step.
3. **Input→output fit** (first-order plant driven by the *recorded* duty `out`) on a rich multi-step
   profile (40/60/120/140 rpm) → poor (fit ≈ 2%, τ implausible).

**Root cause (the steady-state map makes it obvious):** the closed-loop duty `out` sits at
**~0.039 for *every* speed 40–140 rpm** — the operating point is just above the motor's Coulomb
**friction breakaway**, so speed changes ride a razor-thin duty window (~0.004) that is buried under
the **±25–35 rpm velocity-estimate noise** at **100 Hz** telemetry. There is essentially no input
excitation to fit, and the integral-dominated closed loop is plant-insensitive.

### Preliminary (PLACEHOLDER) model — do not trust for design
- From the DIAG open-loop point (0.15 duty → ~866 rpm, from commissioning): **K ≈ 5773 rpm/duty**
  (through origin, ignores friction). **τ ≈ 0.08 s** borrowed from the sibling MG513P30 accel step.
- Written into `mg513_gmr_params.m` as `p.K=5773, p.tau=0.08` **marked placeholder**. Steady-state is
  strongly **Coulomb-offset** (breakaway ≈ 0.035–0.04 duty), so a single linear K is only a local
  approximation — the sibling's accel/decel τ asymmetry (≈80 vs ≈190 ms) applies here too.

## HIL overlay — pipeline demonstrated end-to-end
`hil_overlay(sp=80, gains=baked)` ran on hardware: pushed gains (echo-verified) → captured live
telemetry → computed the model prediction → saved overlay + RMSE
([`docs/sysid/overlay_80_20260707_162506.png`](sysid/overlay_80_20260707_162506.png)). The plot
visually confirms the finding: the **model rises to 80 and holds flat (plant-insensitive)** while the
**hardware shows the ±50 rpm velocity-estimate noise** — RMSE ≈ 55 rpm is noise-/placeholder-dominated,
not a controller defect. The *loop mechanics* (design → push → capture → overlay) work; the
*validation* is weak until the plant model and the velocity estimate improve.

## Recommended next step — open-loop 1 kHz burst logger (the real fix)
The sibling MG513P30 solved exactly this with a firmware **`cap` burst logger** (open-loop `jog <duty>`
→ 1 kHz on-MCU capture of `vel,duty`). Port that to the GMR firmware (a raw-duty command + a 1 kHz
capture buffer). It delivers **two** things at once:
1. **Clean plant ID** — large open-loop duty steps (0.4–0.99, well above breakaway) at 1 kHz →
   `sysid.py` / MATLAB `tfest` (licensed) give real K/τ + the Coulomb/friction structure.
2. The **Phase-2 external-mode seam** (Simulink-as-controller) needs the same raw-duty command.
This is a firmware change → do it **upstream** in `stm32-nucleo-gmr-encoder` + pointer bump (submodule
policy). A quieter velocity estimator (windowed, like the P30 retrofit) would also cut the ±30 noise.

## Reproduce
```bash
# software gates (no hardware)
~/.venvs/mg513-matlab-mcp/bin/python mcp/validate_matlab_vs_sil.py      # S2/S3 bit-exact
# comms-only (no motion)
~/.venvs/mg513-matlab-mcp/bin/python mcp/hil_bridge.py --selftest --port /dev/cu.usbmodem1103
# plant fit from a capture (needs motion go-ahead to capture)
~/.venvs/mg513-matlab-mcp/bin/python mcp/fit_plant_graybox.py docs/sysid/rpm_multistep_*.csv
```

## Data files (`docs/sysid/`)
- `rpm_step_80_sysid_*.csv` — single closed-loop step (0→80).
- `rpm_multistep_*.csv` — 5-step rich profile (40/60/120/140/80) for I/O fitting.
- `rpm_step_80_overlay_*.csv` + `overlay_80_*.png` — the HIL overlay capture + plot.
- **All closed-loop, 100 Hz.** Open-loop 1 kHz captures (pending the burst logger) will supersede these.
