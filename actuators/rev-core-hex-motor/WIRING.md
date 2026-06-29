# WIRING — REV Core Hex Motor (MC33886 + STM32 Discovery)

Wire only with power OFF + discharge wait. Confirm every value against [SPECS.md](SPECS.md).
Board: **STM32F429I-DISC1** (F429ZI). Encoder/motor pinouts confirmed from the REV image.
**Pin-conflict audit CLOSED 2026-06-29** (sources: ST UM1670 + Zephyr F429I-DISC1 board doc): motor/
encoder pins (PB4/PA5/PA7/PC4/PC5) and USART1 (PA9/PA10) are free of SDRAM-FMC / LTDC / SPI5-gyro / USB;
**I2C3 (PA8/PC9) is shared with the on-board STMPE811 touch @ 0x41** → coexist via INA238 @ **0x40**;
**FS1 → PB7** (free, 5 V-tolerant). **MC33886 FS = open-drain, active-LOW, pull-up to 5 V** (idle=HIGH,
fault=LOW). All former `‹…›` placeholders resolved.

## POWER
```
   DC PSU 12 V               Waveshare RPi Motor Driver Board
   (limit ~1.5 A, fused)
     (+) ──[ FUSE ]────────► Power-input terminal +  (VIN, 7–40 V)
     (−) ──────────────────► Power-input terminal −
                                  (board makes its own 5 V via LM2596; motor driven by MC33886)
   REV Core Hex (JST-VH)
     M+ ───────────────────► Motor-A OUTPUT screw terminal  M1
     M- ───────────────────► Motor-A OUTPUT screw terminal  M2
   First test: 12 V, current limit ~1.5 A (stall 4.4 A; board rated 5 A/ch — start limited), fused.
   Meter-verify polarity; share GND with the STM32 (see CONTROL).
```
> ⚠️ **Naming collision:** the board's **M1/M2 SCREW terminals = motor-A OUTPUT** (wire the REV motor
> here), while **M1/M2 on the 40-pin HEADER = direction INPUTS** (CONTROL section). Don't swap them.

## CONTROL — Waveshare RPi Motor Driver Board (2× MC33886), Motor-A (right) channel
Driven from the F429, **not** a Pi (vendor: "treat M1/M2 as digital outputs, PWMA as PWM"). Logic
passes through the board's **74LVC4245AD** translator, so feed the Pi-header positions at 3.3 V **and
supply 3V3 to a Pi 3V3 header pin** (translator A-side ref). Header pins are the **physical** numbers
from the Waveshare wiki. Reuses the proven `st-discovery` DBH-12V map (PWM + 2 dir).
```
   STM32F429 (3.3 V)            Waveshare Pi-header (physical pin)
   PB4 TIM3_CH1 (~20 kHz) ────► PWMA = Pin 37     active-high PWM enable (speed)
   PA5 GPIO out          ────► M1   = Pin 38     direction A
   PA7 GPIO out          ────► M2   = Pin 40     direction B
   3V3                   ────► 3V3  = Pin 1       (74LVC4245AD A-side reference)
   GND                   ────► GND  = Pin 34/39   ◄── single common ground
   PB7 GPIO in (FT)      ◄──── FS1  = board FS1 pad (NOT on the 40-pin header; open-drain, 1k→5 V)
                                         active-LOW: idle=HIGH(~5 V), fault=LOW. PB7 is 5 V-tolerant →
                                         connect direct, NO internal pull-up; trip PWM→0 & latch on LOW.
   12 V PSU              ────► VIN (Power screw terminal)  7–40 V; board buck makes its own 5 V
```
Direction / speed (sign-magnitude):
| M1 | M2 | PWMA | Motor A |
|---:|---:|---|---|
| 1 | 0 | PWM | forward @ duty |
| 0 | 1 | PWM | reverse @ duty |
| x | x | 0 | coast / stop |
> **Power-select switch → OFF** so the board does NOT back-feed 5 V to the controller side (the F429 is
> USB-powered; don't wire the board's 5 V header pin to the F429). FS1 is a separate pad → 5 V-tolerant
> pin or divider; trip PWM→0 on fault. (Motor B / left = M3 Pin 31, PWMB Pin 32, M4 Pin 33 — unused.)

## CURRENT SENSE — TI INA238 (I²C power monitor, external)
```
   board OUT1 (M+) ──►[ Rshunt ~5 mΩ ]──► motor M+     (in-line; INA238 reads SIGNED current)
                        │            │
                  INA238 IN+      INA238 IN-
                  INA238 VBUS ──► 12 V node            (bus-voltage readout, for V·I param-ID)
                  INA238 SCL/SDA ─── I²C ───► STM32 I2C3 (SCL PA8, SDA PC9); board already pulls up
                                       I2C3 for the STMPE811 touch — reuse those, do NOT stack a 2nd set
                                       (verify value via UM1670; target 2.2–4.7k effective). Bus ≤400 kHz.
                  INA238 A0/A1 ─── BOTH to GND → address 0x40 (Table 6-2). STMPE811 touch sits at 0x41
                                       (=A1 GND/A0 VS) → 0x40 is the collision-free pick; touch left idle.
   Rshunt ~5 mΩ → 4.4 A ≈ 22 mV, within INA238 ±40.96 mV high-res range. Kelvin-sense the shunt.
```
> INA238 returns **signed current + bus voltage + die-temp** over I²C → feeds the torque loop, the
> fault trip, and (synchronized V,I) the Simulink param-ID. Use INA238 averaging to reject PWM noise.
> ⚠️ Shunt placement: a PWM'd motor lead has high CM dv/dt — consider low-side return instead; pick on
> the bench. If a fast inner current loop needs more BW than I²C allows, fall back to analog INA240→ADC.

## ENCODER — quadrature (4-pin JST-PH)
```
   REV encoder JST-PH (pins 1→4: Ch B, Ch A, 3.3V, GND)   STM32F429
   pin1 Ch B ──────────────────────────────────────────►   PC5  (reuse st-discovery encoder B)
   pin2 Ch A ──────────────────────────────────────────►   PC4  (reuse st-discovery encoder A)
   pin3 3.3V ──────────────────────────────────────────►   3V3  (encoder is native 3.3 V)
   pin4 GND  ──────────────────────────────────────────►   GND
```
> Reuses the proven `st-discovery` PC4/PC5 encoder pins (already audited free). They're **not** a
> timer-encoder pair, so decode A/B by **EXTI on both edges** and timestamp each edge from a free-running
> **TIM2 (32-bit)** for low-speed velocity (T/M-method). 600 counts/s max is trivial for EXTI; the
> velocity win comes from the timestamps. 288 counts/rev output → see [MODELING.md](MODELING.md).
> (If you prefer hardware encoder mode, relocate to a free TIM CH1/CH2 pair — but **first run the
> SDRAM/LTDC pin-conflict audit**; many timer pins collide with the board's SDRAM/LCD.)

## DO / DON'T
```
  DO    power STM32 (USB) + idle FIRST, then 12 V motor supply; reverse on power-down
  DO    set DIR before commanding PWM; start with the smallest counted move
  DO    keep ONE common ground (STM32 / MC33886 / shunt-amp / encoder)
  DO    monitor MC33886 FS (fault) and trip the PWM to 0 on fault
  DON'T drive the FS line (it is an OUTPUT)
  DON'T exceed MC33886 sustained current at stall — current-limit the PSU, watch heat
  DON'T route 12 V into the STM32 or the encoder (encoder is 3.3 V here)
  DON'T hot-plug the motor or encoder connector with power on
```

## Pin map — proposed (reuses st-discovery; ⚠️ run the SDRAM/LTDC/touch/gyro conflict audit before firmware)
| Function | F429 pin / peripheral | Source / notes |
|---|---|---|
| Motor PWM → board PWMA | `PB4` — TIM3_CH1 (AF2) | reuse (DBH map); ~20 kHz. PB4=NJTRST → flash via SWD only |
| Direction → board M1 | `PA5` — GPIO out | reuse (3.3 V output; DAC-capable pin, irrelevant as digital out) |
| Direction → board M2 | `PA7` — GPIO out | reuse |
| Fault ← board FS1 | `PB7` — GPIO in (FT, floating) | ✅ free + 5 V-tolerant. MC33886 FS = OD active-LOW, 1k→5 V; LOW=fault → trip PWM=0 |
| Encoder A ← Ch A | `PC4` — GPIO/EXTI (pull-up) | reuse; native 3.3 V |
| Encoder B ← Ch B | `PC5` — GPIO/EXTI (pull-up) | reuse |
| Velocity timebase | `TIM2` (32-bit) free-run | edge timestamps (T/M-method) |
| Current sense (INA238) | `I2C3` — SCL `PA8` / SDA `PC9` | shared w/ on-board STMPE811 touch @ 0x41 → INA238 @ **0x40** (A0=A1=GND); reuse board's I2C3 pull-ups, ≤400 kHz |
| Control ISR | `TIM6`/`TIM7` (basic) @ 1 kHz | PID loop |
| UART telemetry | `PA9`/`PA10` — USART1 | reuse; 115200 8N1 |
| LCD telemetry (optional) | LTDC (board-reserved) | reuse st-discovery LCD-plot for live traces |

> Already proven free in st-discovery: PB4, PA5, PA7, PC4, PC5, PA9, PA10. **Audit closed 2026-06-29**
> (cross-checked against `rnd-southerniot/st-discovery` → `docs/stm32f429i-disc1-pin-mapping.md`, the
> local point-of-truth; + ST UM1670 / Zephyr board doc): **FS1 → PB7** (free, 5 V-tolerant). **Rejected
> alt PA15** — it's the STMPE811 touch INT (EXTI15_10), already taken. (PB7 is board-free; st-discovery's
> `balance-mvp` only uses it as I2C1-SDA for an optional accel we are NOT adding — so it's ours for FS.)
> **I2C3 PA8/PC9** is the
> STMPE811 touch bus (@0x41) — INA238 coexists at **0x40**, touch left un-driven. Per st-discovery
> `AGENTS.md`, this is the final map — carry it verbatim into the firmware README.
> **Bench checks still required:** confirm PB7 reads HIGH idle / LOW on a forced FS trip; meter the board's
> I2C3 pull-up value before adding the INA238 module; verify FS1 pad is the channel-A fault (not B).
