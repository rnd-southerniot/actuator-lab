# NEXT SESSION — MG513/GMR + DBH-12V + NUCLEO-F401RE

Handoff note. Updated **2026-07-07** — **Phases 0–4 PASS on-bench** (comms, encoder, open-loop,
closed-loop RPM). Three LOCAL firmware fixes applied in `src/` (see below). P5–7 remain.

## Where we are
| # | Item | State |
|---|---|---|
| 1 | actuator-lab docs + submodule firmware | ✅ |
| 2 | Phase 1 comms / Phase 2 encoder | ✅ bench-confirmed (5/5; CW=+, reverses, zero at rest) |
| 3 | Phase 3 open-loop + direction | ✅ DIAG +duty→+866 rpm (after `motor.c` sign fix) |
| 4 | Phase 4 closed-loop RPM | ✅ **kp=0.0015 ki=0.01 kd=0** baked as boot defaults; ss_err<0.5 rpm @80/150; band 0–200 |
| 5 | Phase 5–7 (position, repeatability, faults) | ⛔ needs 12 V + fresh written go-ahead |

## ⚠️ Local firmware fixes in `src/` (NOT committed upstream — kept local per Arif)
Rebuild needs **`make clean && make`** (plain `make` linked a stale object once). Then OpenOCD `program … verify reset`.
1. **`Core/Src/motor.c`** — `Motor_SetOutput()` `duty = -duty;` (direction: +duty→+RPM/+encoder).
2. **`Core/Src/main.c`** — added `HAL_UART_ErrorCallback` (clears UART ORE + re-arms RX). Fixes the
   **command-RX wedge** (was: board silently ignores all commands until ST-LINK reset).
3. **`Core/Src/controller.c`** — RPM `PID_Init` boot defaults `0.0015f, 0.01f, 0.0f` (was 0.5/0.1/0.01,
   ~300× too hot → bang-bang). Baked because **runtime flash-save is broken** (IWDG resets mid sector-erase).

## Exact next steps (Phase 5+)
1. **Power on** (controller/USB first, then 12 V current-limited ~2–3 A) and **get a fresh written go-ahead**.
2. **Phase 4 cal still open:** bench-verify **60 000 cnt/rev** (500 PPR×4×30) against a shaft mark, multi-rev
   method (sibling gearbox measured 28:1, not 30 — don't assume). Also settle whether telemetry `rpm` is
   motor- or output-shaft (MOTOR_MAX_RPM=200 in that unit).
3. **Phase 5 position mode** (`CMD_SET_POSITION` / trapezoidal profile), **Phase 6 repeatability**
   (N out-and-back, drift), **Phase 7 faults** (overcurrent EN-gated, stall, ESTOP/brake, IWDG, low-batt).
4. **Command hygiene:** commands land reliably now (wedge fixed), but keep cadence sane. Use the
   `hw_test` `SerialFixture` path (auto-clear-fault + atexit ESTOP). Bench scripts in
   `scratchpad/` (`tune.py`, `verify_boot.py`) are throwaway helpers.

## Gotchas (carried in)
- **Firmware fixes are LOCAL** — the `src/` submodule shows a dirty tree (motor.c, main.c, controller.c);
  pointer intentionally NOT bumped. To persist: commit in `rnd-southerniot/stm32-nucleo-gmr-encoder` + bump.
- Encoder 5 V (not 3.3). PA4 (CT) ≤ 3.3 V; CT floats without 12 V (overcurrent gated on EN).
- Relay **autotune bails to FAILED** on this plant → leaves gains unchanged; hand-tune is the proven path.
- 18650 sags under load — a stiff bench PSU is better for current/characterization work.
- **Don't confuse with [mg513p30-hall-motor](../../mg513p30-hall-motor/)** (Hall 13-PPR, F429+MC33886).

## One-line resume
```bash
cd /Users/robotics/Developer/projects/robotics/actuator-lab/actuators/mg513-gmr-nucleo-f401/src/firmware && \
  make clean && make && openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program build/dc-motor-pid.elf verify reset exit"
```
