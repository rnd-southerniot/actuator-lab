# MG513/GMR MATLAB MCP server — contract

A per-actuator FastMCP server (`matlab_mcp_server.py`) that drives the MATLAB/Simulink
model-based-design + live hardware-in-the-loop (HIL) workflow for the MG513/GMR rig
(Axis A). See the session plan for the full architecture.

## Runtime
- **Interpreter:** `~/.venvs/mg513-matlab-mcp/bin/python` (python3.12 venv; the MATLAB
  Engine API cannot load the 3.14 system interpreter).
- **MATLAB:** R2026a at `/Applications/MATLAB_R2026a.app`; engine `matlabengine==26.1`
  installed from the local dist. One **persistent** engine session is started lazily on the
  first MATLAB-touching tool call (base workspace persists across calls).
- **Transport:** stdio (for Claude Code). Registered in the repo-root `.mcp.json` as
  `mg513-matlab`, env `MG513_PORT` (default `/dev/cu.usbmodem1103`).
- **Serial ownership:** from Phase C the server (via `hil_bridge.py`) is the **sole** owner
  of the board's serial port. Stop `src/host/server.py` and any dashboard bridge first.

## Tools

### MATLAB-side (no hardware)
| Tool | Signature | Notes |
|---|---|---|
| `matlab_eval` | `(code) -> str` | eval statements in the persistent session; base ws persists. |
| `run_script` | `(path) -> str` | addpath + run a `.m` (abs or relative to `matlab/scripts`). |
| `load_model` | `(slx="mg513_gmr_plant") -> str` | `load_system`; returns block list. |
| `check_toolboxes` | `() -> {simulink,control,sysid,simscape,...}` | `license('test',…)`. |
| `set_gains` | `(kp,ki,kd) -> dict` | push gains to MATLAB base ws (model only; NOT the board). |
| `sim_model` | `(setpoint,dur,kp,ki,kd,K,tau,mode) -> {t,rpm_pred,out_pred}` | closed-loop model prediction (needs Phase-B `mg513_sim_closed`). |
| `sysid_from_csv` | `(csv_path,gear_ratio=30) -> {K,tau,r_squared,…}` | wraps host `sysid.identify_from_csv`. |

### Hardware-side (Phase C+; **motion go-ahead required** for the spinning ones)
| Tool | Signature | Motion? |
|---|---|---|
| `estop` | `() -> ok` | safe (stops motion). |
| `clear_fault` | `() -> ok` | safe. |
| `push_gains_to_board` | `(kp,ki,kd) -> {echoed_kp,ki,kd}` | motion-adjacent — SET_PID + telemetry-echo verify. |
| `hil_capture` | `(setpoint,dur,mode) -> {csv_path,t,rpm,out,rpm_sp}` | **spins the motor.** |
| `hil_overlay` | `(setpoint,dur,kp,ki,kd,K,tau,mode) -> {csv_path,png_path,rmse}` | **spins the motor.** |

Hardware tools lazy-import `hil_bridge` and return an `{"error": …}` dict (never crash the
server) if the bridge/board is unavailable.

## Safety
- The model must match the **as-flashed local firmware** (motor.c sign fix, UART-ORE fix,
  baked gains kp=0.0015/ki=0.01/kd=0, trap-vel 30) — not upstream.
- `hil_capture`/`hil_overlay`/`push_gains_to_board` require a **written motion go-ahead** +
  12 V current-limited (2–3 A, fused), motor secured/shaft clear (`SAFETY-CHECKLIST.md`).
- The bridge inherits the hw_test `SerialFixture` atexit/SIGINT **ESTOP guarantee** +
  auto-clear-fault.

## Smoke tests
- **S1:** `~/.venvs/mg513-matlab-mcp/bin/python -c "import matlab.engine as e; m=e.start_matlab(); print(m.sqrt(4.0)); m.quit()"`
- **S4:** `claude mcp list` → `mg513-matlab` present.
- **S5 (Phase C, comms-only):** `hil_bridge.py --selftest --port $MG513_PORT`.
