# NEXT SESSION — MATLAB/Simulink MBSD + live HIL

Handoff for the **model-based system design** workflow (Simulink plant + faithful PID, driven via a
**MATLAB MCP server**, with a live hardware-in-the-loop loop). Built + demonstrated 2026-07-07 on the
MG513/GMR rig. Full detail: [MODELING.md](MODELING.md). Plant-ID is the open blocker (data-limited).

## Where it stands
| Phase | State |
|---|---|
| A — MATLAB MCP server | ✅ built (`../mcp/matlab_mcp_server.py`, 12 tools, persistent `matlab.engine`). Registered in repo-root `.mcp.json` as `mg513-matlab` — **⏸ needs one-time approval** (`run claude` → approve). |
| B — Simulink plant + faithful PID | ✅ **bit-exact** to firmware SIL (max\|Δu\|=0, RMSE=0). `mg513_pid_fcn.m`, `mg513_sim_closed.m`, `mg513_gmr_params.m`, design `mg513_gmr_plant.slx`. Gate: `../mcp/validate_matlab_vs_sil.py`. |
| C — serial HIL bridge | ✅ `../mcp/hil_bridge.py` (reuses COBS/CRC + ESTOP guarantee). Comms-only selftest passes. |
| D — plant identification | ⚠️ **data-limited** — see below. Placeholder K/τ in `mg513_gmr_params.m`. |
| E — live HIL overlay | ✅ pipeline demonstrated end-to-end on hardware (push gains → capture → model → overlay PNG + RMSE). |

**Environment:** MATLAB R2026a (Simulink, Control, System-ID, Simscape all licensed). Engine venv at
`~/.venvs/mg513-matlab-mcp` (python3.12 — the 3.14 system Python can't host the engine). Deps in
`../mcp/requirements.txt` (+ engine from `/Applications/MATLAB_R2026a.app/extern/engines/python`).

## The blocker (and the fix)
Closed-loop 100 Hz telemetry **cannot identify the plant**: at the operating point the duty sits at
~0.039 for *every* speed 40–140 rpm (friction breakaway), and with an integrator the closed loop
reaches setpoint independent of K; the ±30 rpm velocity noise buries the rest. Three fits all failed.
→ **The real fix is an open-loop 1 kHz burst logger** (a raw-duty command + capture buffer, like the
sibling MG513P30's `cap`). Large open-loop duty steps (0.4–0.99) at 1 kHz give clean K/τ via `sysid.py`
/ MATLAB `tfest`, **and** are the seam the Phase-2 "Simulink-as-controller" external mode needs.

## Exact next steps
1. **Approve the MCP server** (`claude` → approve `mg513-matlab`) so the MATLAB/Simulink tools are usable
   in-session. Smoke test: `~/.venvs/mg513-matlab-mcp/bin/python mcp/validate_matlab_vs_sil.py` (S2/S3).
2. **Add the open-loop burst logger to firmware** (now easier — the dual-axis refactor gave a clean
   `MotorHandle`/raw-duty path). Add a `CMD_SET_DUTY` (open-loop, EN-gated) + a 1 kHz capture, per axis.
   This is the same change that unlocks external-mode HIL. Do it **upstream** in the submodule.
3. **Real plant ID** — capture open-loop steps → `sysid.py`/`tfest` → write K/τ into `mg513_gmr_params.m`
   (replaces the placeholder). Then the HIL overlay (`hil_overlay`) becomes a true validation.
4. **Phase-2 external mode** — Simulink computes the control action, streams it to the board via the new
   raw-duty command (~50–100 Hz). Design seam is documented in MODELING.md.
5. **Dual-axis extension** — both motors are now commissioned; the bridge/MCP can target either axis
   (`hil_bridge` capture already axis-filters; add an `axis` param to the HIL tools).

## Files (present in this actuator folder)
- `../mcp/`: `matlab_mcp_server.py`, `hil_bridge.py`, `validate_matlab_vs_sil.py`, `fit_plant_graybox.py`,
  `requirements.txt`, `MCP_SERVER_CONTRACT.md`.
- `../matlab/`: `scripts/` (`mg513_pid_fcn`, `mg513_pid_gains`, `mg513_pid_state`, `mg513_pid_run`,
  `mg513_gmr_params`, `mg513_sim_closed`, `mg513_build_slx`, `validate_pid_vs_sil`) + `models/mg513_gmr_plant.slx`.
- `MODELING.md` + `sysid/` (captured CSVs + overlay PNG).
- Repo-root `.mcp.json`. MATLAB build artifacts (`*.slxc`, `slprj/`) are git-ignored.

## Gotchas
- The Simulink model matches the **as-flashed** firmware (baked gains 0.0015/0.01/0, dt=1e-3, N=10,
  b=1/c=0, ±1 clamp). Re-fit if gains change.
- HIL bridge must be the **sole** serial owner — stop `src/host/server.py` first.
- `hil_capture`/`hil_overlay` **spin the motor** → need a fresh motion go-ahead + 12 V.
