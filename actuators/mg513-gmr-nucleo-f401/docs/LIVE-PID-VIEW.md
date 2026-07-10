# Live PID View — MG513/GMR (MATLAB / Simulink + hardware)

Watch the closed-loop PID **control story** — setpoint · measured rpm · error ·
control duty — evolve over time instead of reading a post-run report. Two views:

| View | Shows | Runtime | Motor |
|---|---|---|---|
| **A. Simulink live sim** | the *modelled* PID (design plant + tunable PID) | pure **simulation**, 1× wall-clock | no |
| **B. Hardware live plot** | the *real* firmware PID + observer + motor | **live hardware**, ~100 Hz | **yes — gated** |

> Both are **non-destructive**: they never change the validated gains
> (`kp=0.0015, ki=0.01, kd=0`) or overwrite the model/firmware. Use them to
> *watch*, not to retune.

---

## Prerequisites

- **MATLAB R2026a** at `/Applications/MATLAB_R2026a.app` (View A). A shell alias is set:
  `alias matlab='/Applications/MATLAB_R2026a.app/bin/matlab'` (in `~/.zshrc`).
- **Python venv** `~/.venvs/mg513-matlab-mcp` with matplotlib + pyserial (View B).
- For View B: the board on `/dev/cu.usbmodem1103`, and a **motion go-ahead**
  (motor secured, shaft/path clear, 12 V current-limited & fused).
- Paths below assume the actuator root:
  `~/Developer/projects/robotics/actuator-lab/actuators/mg513-gmr-nucleo-f401`

---

## View A — Simulink live simulation (no motor)

**Model:** `matlab/models/mg513_gmr_plant.slx`
`Step → Rate Limiter → Σ(error) → Discrete PID → plant K/(τs+1) → encoder LPF → feedback`.
**Helper:** `matlab/scripts/mg513_live_sim.m` — logs 4 signals to the **Simulation
Data Inspector (SDI)** and runs at **1× real-time** (Simulation Pacing) so the plot
fills live.

### Run it
This is **MATLAB code — run it inside MATLAB, not in the zsh terminal.**

**Option 1 — from a terminal (opens the MATLAB desktop and runs it):**
```zsh
matlab -r "addpath('$HOME/Developer/projects/robotics/actuator-lab/actuators/mg513-gmr-nucleo-f401/matlab/scripts'); mg513_live_sim(150,4)"
```
Use `-r` (keeps the desktop). **Do not use `-batch`** — it's headless and the live
SDI window won't appear.

**Option 2 — inside the MATLAB app:** launch MATLAB, then at the `>>` prompt:
```matlab
addpath('.../mg513-gmr-nucleo-f401/matlab/scripts')   % once per session
mg513_live_sim(150, 4)      % setpoint = 150 rpm, run = 4 s
mg513_live_sim               % defaults: 80 rpm, 3 s
```

The **Simulation Data Inspector** opens and fills over `dur` seconds with
`setpoint / measured_rpm / error / control_duty`, each on its own auto-scaled axis.
Re-run with a new setpoint to overlay/compare runs in SDI.

### No-code fallback (measured rpm only)
`open_system('mg513_gmr_plant')` → toolbar **Run ▾ → Simulation Pacing → Enable,
1.0 s/s** → **Run** → double-click the **Scope** block. Shows measured rpm live.

---

## View B — Hardware live plot (real controller, **spins the motor**)

**Helper:** `mcp/mg513_live_hil.py`. Streams board telemetry and plots
setpoint + measured rpm (top), control duty + error (bottom) in real time. Uses the
verified command helpers so the setpoint always lands. **Closing the window or
Ctrl-C sends ESTOP and disconnects.**

### Run it — from the **actuator root** (not `matlab/scripts`)
```zsh
cd ~/Developer/projects/robotics/actuator-lab/actuators/mg513-gmr-nucleo-f401
~/.venvs/mg513-matlab-mcp/bin/python mcp/mg513_live_hil.py --axis A --rpm 100
```
No venv activation needed — calling the venv's `python` by full path is enough.

| Flag | Default | Meaning |
|---|---|---|
| `--axis` | `A` | `A` or `B` |
| `--rpm` | `80` | setpoint (**use 100–150**, see note) |
| `--dur` | `15` | run seconds (`0` = until you close the window) |
| `--window` | `6` | rolling x-axis window (s) |

### ⚠️ Operating-floor note (important)
**80 rpm is the bottom edge of the validated band and hunts under real load** — in
the mechanism it goes into a stick-slip / backlash **limit cycle** (measured rpm
swinging ±50, duty rail-to-rail). This is the known `<`75–80 rpm marginality, **not
a gain fault** — do not retune. **Run the application at ≥100 rpm.** 100 and 150
track cleanly; 80 is "edge, expect hunting."

---

## Signal reference (both views)

| Signal | Simulink block | Hardware field | What good looks like |
|---|---|---|---|
| Setpoint | `RL/1` (rate-limited) | `rpm_sp` | flat at target after the ramp |
| Measured rpm | `LPF/1` | `rpm` | tracks setpoint within the noise floor (±15–22 rpm) |
| Error | `Err/1` | `rpm_sp − rpm` | ≈0 mean; small ripple |
| Control / duty | `PID/1` | `out` | small & steady (~0.05); **not** swinging to ±rails |
| Integrator / P-I-D terms | — | — | not exposed by the stock PID block; reconstruct as `ki·∫error dt` if needed |

**Healthy vs limit cycle:** healthy = measured hugs the dashed setpoint, duty sits
quiet near its operating value. Limit cycle = measured sawtooths tens of rpm and
duty swings rail-to-rail in phase with the error (what 80 rpm loaded shows).

---

## Troubleshooting

| Symptom | Cause → fix |
|---|---|
| `zsh: unknown file attribute: 1` | You ran a MATLAB command (`mg513_live_sim(150,4)`) in the **zsh** shell. Run it **inside MATLAB** (or via `matlab -r "…"`). |
| `matlab: command not found` | Alias not loaded → open a new terminal or `source ~/.zshrc`; or use the full path `/Applications/MATLAB_R2026a.app/bin/matlab`. |
| SDI window doesn't appear | You used `matlab -batch` (headless). Use `matlab -r "…"` to keep the desktop. |
| `can't open file '.../matlab/scripts/mcp/mg513_live_hil.py'` | Wrong directory — you ran the Python from `matlab/scripts`. `cd` to the **actuator root** first, or use the absolute path. |
| `could not open/ping /dev/cu.usbmodem1103` | Board off, or another process owns the port (a stale MCP server / another plot). Close it, or `pkill -f matlab_mcp_server`. |
| Hardware plot: “mode entry failed (RX drop)” | A dropped `SET_MODE` under RX load — just re-run; the verified helpers retry. |
| Measured rpm limit-cycles at 80 | Expected at the floor under load — **run ≥100 rpm** (see note above). |

---

## What these tools are / aren't

- **View A is pure simulation** (design model, stock tunable PID, legacy encoder
  LPF). Good for watching PID *shape* and tuning experiments — **not** bit-exact to
  firmware. The firmware-faithful *numeric* predictor is `mg513_sim_closed.m`
  (used by the HIL overlay), not a live plot.
- **View B is the real controller** on the motor — the authentic "watch the PID
  behave" view.
- Neither changes gains or the known-good setup. Files added for these views:
  `matlab/scripts/mg513_live_sim.m`, `mcp/mg513_live_hil.py`.

*Related:* [MODELING.md](MODELING.md) (model/HIL background) ·
[SAFETY-CHECKLIST.md](../SAFETY-CHECKLIST.md) (motion gate).
