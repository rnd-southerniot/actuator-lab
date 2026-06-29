# SPECS — REV Core Hex Motor  (canonical verified interface)

> Values below are **vendor-cited**; they become bench-verified during commissioning (mark ✅ then,
> recorded in [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md)).
> **Source:** REV Robotics — Core Hex Motor (REV-41-1300),
> <https://docs.revrobotics.com/duo-build/motion/motors/core-hex-motor>, accessed 2026-06-29.

## Identity
- Part: **REV Core Hex Motor** (REV-41-1300) — 12 V brushed DC gearmotor, **72:1** gearbox,
  **5 mm female-hex** output (accepts REV 5 mm hex shafts), 90° body orientation, ~7 oz.
- Class: PWM H-bridge **DC servo** (bidirectional PWM + quadrature encoder). NOT pulse/dir, NOT bus.

## Power (motor) — drives driver selection
- Nominal: **12 V DC**.
- **Stall current: 4.4 A** ← sizing number for the driver + supply fuse/limit.
- Free speed: **125 RPM** (output). Stall torque: **3.2 N·m** (output).
- No-load / free-running current: `⏳ not in vendor summary` — measure at Phase 2.
- Connector: **2-pin JST-VH** — pinout **`M- · M+`** (per REV pinout image).

## Control interface
- Method: **bidirectional PWM via external H-bridge.** Chosen driver: **MC33886** — realized as the
  **Waveshare "RPi Motor Driver Board"** (**2× MC33886**; on-board LM2596 5 V reg, **74LVC8T245**
  3.3↔5 V level translator, Schottky flyback diodes, per-channel FS fault, 2 A self-recovery fuse).
  (Translator PN per the **board schematic** — Waveshare's wiki product text mislabels it "74LVC4245AD";
  the schematic's "Level Translator" block is 74LVC8T245. Both are 8-bit dual-supply VCCA/VCCB
  transceivers, but cite the schematic.)
  Input **7–40 V**; **single-motor output up to 5 A** (vendor) → **4.4 A stall is within rating**
  (still current-limit + watch heat). M1/M2 = direction, PWMA = active-high PWM enable. (Alt on hand:
  BTS7960 43A. Earlier "TB6612FNG" label was wrong — this board is MC33886-based.) See [WIRING.md](WIRING.md).
- **Fault flag (FS):** MC33886 **FS = open-drain, active-LOW**, external pull-up to **5 V** (board's
  1 k); reports under-voltage / short / over-temp. **Idle/OK = HIGH (~5 V), fault = LOW.** Read on a
  5 V-tolerant MCU input (**PB7**), no internal pull-up; firmware trips PWM→0 and latches on LOW.
  Source: NXP **MC33886** datasheet rev 10.0 (01/2014), FS pin description.
- MCU: **STM32F429I-DISC1** (STM32F429ZI, 180 MHz, on-board 2.4" LCD for bench telemetry). Firmware
  follows the `rnd-southerniot/st-discovery` repo (arm-none-eabi + Makefile + st-flash).
- Logic: driver inputs from STM32 **3.3 V** (PWM + DIR, EN, /D2; FS fault read-back). No on-motor logic.
- PWM frequency: target **~20 kHz** (above audible) — confirm against MC33886 slew/transition limits.

## Feedback / encoder
- Built-in **magnetic quadrature** encoder; works at **3.3 V or 5 V** logic (power from STM32 3V3).
- Resolution: **4 counts/rev (motor) → 288 counts/rev (output)** (×72 gear). Quadrature A/B.
- **Position resolution = 360°/288 ≈ 1.25°/count** at the output.
- ⚠️ **Velocity is the hard part:** 125 RPM ≈ 2.08 rev/s → **~600 counts/s max**. At a 1 kHz loop
  that is **<1 count/sample** — counts-per-sample velocity is unusable; use **edge-timing
  (T/M-method) via timer input capture**. See [docs/MODELING.md](docs/MODELING.md).
- Backlash: geared output has non-trivial backlash → model it; limits position accuracy.
- Connector: **4-pin JST-PH**, pinout (L→R per REV image): **`Ch B · Ch A · 3.3V · GND`**. Native
  3.3 V → wires straight to the F429 (no level shift).

## Current sensing (for closed-loop / system-ID — per project goal)
- The Waveshare board exposes no motor current. Sense it externally with a **TI INA238** — a **16-bit
  I²C power monitor** (⚠️ INA**238**, not INA228; ±40.96/±163.84 mV shunt FS, 85 V common-mode, signed
  current, + bus-voltage + die-temp). Across a **~5 mΩ** inline shunt (4.4 A → ~22 mV, within ±40.96 mV
  high-res range). Read over **I²C3** (PA8/PC9, shared with the board's STMPE811 touch). **Address
  0x40** (A1=GND, A0=GND, per INA238 datasheet Table 6-2) — the collision-free pick vs the touch
  controller at **0x41**; bus ≤400 kHz, reuse the board's existing I2C3 pull-ups.
- Gives **synchronized V + I** → ideal for Simulink electrical-param ID (Ra, La, Kt). ⚠️ I²C read
  latency caps a *fast* inner current loop; fine for system-ID + a moderate torque loop + protection.

## Config switches / parameters
- None on the motor. All control + tuning lives in the STM32 firmware / Simulink model.

## Verified-on-bench
- None yet — **Phases 0–7 pending** (no go-ahead, no motion). See COMMISSIONING-LOG.md / RESULTS.md.
