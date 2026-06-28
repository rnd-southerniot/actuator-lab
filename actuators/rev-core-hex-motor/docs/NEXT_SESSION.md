# NEXT SESSION — REV Core Hex Motor

Handoff note. Written 2026-06-29 (scaffold only — no hardware yet). Read first, then resolve the TBDs.

## Where we are
| # | Item | State |
|---|---|---|
| 1 | Driver = Waveshare RPi Motor Driver Board (2× MC33886); MCU = STM32F429I-DISC1 | ✅ |
| 2 | Doc scaffold (SPECS/WIRING/SAFETY/MODELING/LOG) | ✅ 2026-06-29 |
| 3 | Motor/encoder pinouts (REV image) + proposed F429 pin map (reuses st-discovery) | ✅ / ⏳ audit |
| 4 | Firmware (st-discovery Makefile project) | ⛔ not written |
| 5 | Bench bring-up (Phases 0–7) | ⛔ not started |
| 6 | Simulink model + system-ID | ⛔ not started |

## Exact next steps (no motion — desk + Phase 0–2)
### Step A — close the wiring (desk)
- Run the **SDRAM/LTDC/touch/gyro pin-conflict audit** (st-discovery `AGENTS.md`) for the NEW pins:
  **FS1** (5 V-tolerant in) and **I2C3** (PA8/PC9 — the board's STMPE811 touch bus; coexist via a
  non-colliding INA238 address or drop touch). Motor/encoder pins (PB4/PA5/PA7/PC4/PC5) already proven free.
- Get the **Waveshare RPi-header pin numbers** for PWMA/IN1/IN2/FS1 + the **IN/PWM truth table** (wiki).
- Pick the **INA238 shunt (~5 mΩ)** + I²C address; decide shunt placement (motor lead vs low-side).

### Step B — firmware (in this folder's `test/`)
Author the project **here** as an st-discovery-style Makefile build (CubeMX + Makefile + `st-flash`;
reuse the DBH-12V PWM/IN1/IN2 + PC4/PC5 encoder map; model the `balance-mvp` control-loop structure).
Pieces: TIM3 PWM ~20 kHz, **EXTI A/B decode + TIM2 edge-timestamp velocity**, **INA238 current/V over
I2C3**, TIM6/7 1 kHz PID ISR, FS-trip, USART1 telemetry, LCD live plot. **Boots PWM = 0.**

### Step C — Phases 0–2 (read-only / first energize)
Wire per WIRING.md, current-limit ~1.5 A. Encoder hand-check (≈288 counts/out-rev), FS clear, ADC scale.
**Phases 3–7 need written go-ahead.**

## Gotchas (anticipated — confirm on the bench)
- **Velocity from 288 CPR is coarse** — <1 count/sample at 1 kHz near top speed. Use edge timestamps
  (TIM2), not counts/sample. (Core design constraint — see MODELING.md.)
- **Driving a Pi HAT from the F429:** feed the RPi-header logic pins at 3.3 V **and supply 3V3 to the
  board** (74LVC8T245 VCA ref); FS1 is pulled to **5 V** → 5 V-tolerant pin or divider.
- **Board "ensured 3 A" < 4.4 A stall** — current-limit, don't sustain stall, trip on FS.
- Gearbox **backlash** dominates small-signal position accuracy — model it.
- Encoder + its supply are **3.3 V** — never 5 V/12 V.

## One-line resume
```bash
# desk: open the scaffold and resolve TBDs
cd /Users/robotics/Developer/projects/robotics/actuator-lab/actuators/rev-core-hex-motor && $EDITOR WIRING.md docs/MODELING.md
```
