# NEXT SESSION — REV Core Hex Motor

Handoff note. Written 2026-06-29 (scaffold only — no hardware yet). Read first, then resolve the TBDs.

## Where we are
| # | Item | State |
|---|---|---|
| 1 | Driver = Waveshare RPi Motor Driver Board (2× MC33886); MCU = STM32F429I-DISC1 | ✅ |
| 2 | Doc scaffold (SPECS/WIRING/SAFETY/MODELING/LOG) | ✅ 2026-06-29 |
| 3 | Motor/encoder pinouts (REV image) + F429 pin map (reuses st-discovery) | ✅ audit closed 2026-06-29 |
| 4 | Firmware (st-discovery Makefile project) | ⏳ Steps 1–3 ✅ compile (Step 3 = INA238 + arm/fault FSM + FS-trip + gated console + 1 kHz PID; boots DISARMED/PWM=0; expert-reviewed). Step 4 (LCD plot) optional. Bench Phases 0–7 next. |
| 5 | Bench bring-up (Phases 0–7) | ⛔ not started |
| 6 | Simulink model + system-ID | ⛔ not started |

## Exact next steps (no motion — desk + Phase 0–2)
### Step A — close the wiring (desk) ✅ DONE 2026-06-29
- ✅ **Pin-conflict audit closed** (ST UM1670 + Zephyr F429I-DISC1 board doc): motor/encoder/USART1
  pins free; **I2C3 PA8/PC9 = STMPE811 touch bus @0x41** → INA238 coexists at **0x40**; **FS1 → PB7**
  (free, 5 V-tolerant). Full map in [WIRING.md](../WIRING.md).
- ✅ Waveshare header pins + truth table resolved: PWMA=Pin37, M1=Pin38, M2=Pin40, 3V3=Pin1, GND=Pin34/39.
- ✅ **FS1 polarity:** MC33886 FS = open-drain, active-LOW, 1k→5 V (idle=HIGH, fault=LOW) → PB7 floating
  input, trip PWM→0 on LOW. (NXP MC33886 datasheet rev 10.0.)
- ✅ **INA238:** ~5 mΩ shunt, **address 0x40** (A0=A1=GND, Table 6-2). Shunt placement (motor-lead vs
  low-side return) deferred to the bench — pick per PWM CM-noise. Reuse board's I2C3 pull-ups (≤400 kHz).
- ⏳ **Bench-confirm (Phase 0–2):** PB7 HIGH idle / LOW on forced FS trip; meter board I2C3 pull-up before
  adding INA238; verify FS1 pad = channel-A fault.

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
- **Board rated 5 A/ch** (MC33886) — 4.4 A stall is within rating but close; current-limit + trip on FS.
- **Naming collision:** board M1/M2 **screw terminals = motor OUTPUT**; M1/M2 **header pins = direction INPUT**.
- Gearbox **backlash** dominates small-signal position accuracy — model it.
- Encoder + its supply are **3.3 V** — never 5 V/12 V.

## One-line resume
```bash
# Step A done; st-discovery cloned at ~/Developer/projects/robotics/st-discovery (untracked reference).
# NEXT = Step B (firmware): scaffold test/ by mirroring firmware/balance-mvp-f429i-disc1 (Makefile +
# linker + startup + HAL config), apply the WIRING.md pin map, boot PWM=0. SHARED_DIR HAL lives in
# st-discovery/firmware/lcd-hello-f429i-disc1/Drivers (balance-mvp's Makefile points there).
cd /Users/robotics/Developer/projects/robotics/actuator-lab/actuators/rev-core-hex-motor && $EDITOR WIRING.md test/README.md
```
