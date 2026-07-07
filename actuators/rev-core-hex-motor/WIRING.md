# WIRING — REV Core Hex Motor (MC33886 + STM32 Discovery)

Wire only with power OFF + discharge wait. Confirm every value against [SPECS.md](SPECS.md).
Block diagram: [docs/WIRING-DIAGRAM.md](docs/WIRING-DIAGRAM.md).
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
passes through the board's **74LVC8T245** translator, so feed the Pi-header positions at 3.3 V **and
supply 3V3 to a Pi 3V3 header pin** (translator A-side / VCCA ref). Header pins are the **physical** numbers
from the Waveshare wiki. Reuses the proven `st-discovery` DBH-12V map (PWM + 2 dir).

```
   STM32F429 (3.3 V)            Waveshare Pi-header (physical pin)
   PB4 TIM3_CH1 (~1 kHz) ────► PWMA = Pin 37     active-high PWM enable (speed)
   PA5 GPIO out          ────► M1   = Pin 38     direction A
   PA7 GPIO out          ────► M2   = Pin 40     direction B
   3V3                   ────► 3V3  = Pin 1       (74LVC8T245 A-side / VCCA reference)
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

## CURRENT SENSE — Adafruit INA238 breakout (#6349, onboard 15 mΩ shunt)

The module carries the motor current through its **onboard 15 mΩ 0.1%** shunt — **no external shunt,
no Kelvin wiring.** Put the breakout **in-line in the board's M1 output lead** via its `VIN+ / VIN-`
terminal block; talk to it over I²C (STEMMA QT or header).

```
   board M1 (Motor-A OUT) ──► INA238 VIN+ ─[onboard 15 mΩ]─ VIN- ──► REV motor M+
   board M2 (Motor-A OUT) ──────────────────────────────────────────► REV motor M-

   INA238 SCL ──► STM32 PA8 (I2C3)    reuse the board's I2C3 pull-ups (touch bus); ≤400 kHz
   INA238 SDA ──► STM32 PC9 (I2C3)
   INA238 VCC ──► 3V3  (module runs 3–5 V)     INA238 GND ──► GND  (single common ground)
   INA238 addr ─ default 0x40   (STMPE811 touch = 0x41, left idle)
```

> Reads **signed current + bus voltage (internal) + die-temp** over I²C → torque loop + fault trip +
> (synchronized V,I) Simulink param-ID. ⚠️ **Set ADCRANGE = 0 (±163.84 mV, up to 10 A):** 4.4 A ×
> 15 mΩ = 66 mV exceeds the ±40.96 mV (2.75 A) range and would **clip at stall**. The M1 lead is
> PWM-switched (high CM dv/dt) and reverses → use INA238 averaging; if a fast inner current loop needs
> more BW than I²C allows, fall back to an analog INA240→ADC.

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
| Motor PWM → board PWMA | `PB4` — TIM3_CH1 (AF2) | reuse (DBH map); ~1 kHz. PB4=NJTRST → flash via SWD only |
| Direction → board M1 | `PA5` — GPIO out | reuse (3.3 V output; DAC-capable pin, irrelevant as digital out) |
| Direction → board M2 | `PA7` — GPIO out | reuse |
| Fault ← board FS1 | `PB7` — GPIO in (FT, floating) | ✅ free + 5 V-tolerant. MC33886 FS = OD active-LOW, 1k→5 V; LOW=fault → trip PWM=0 |
| Encoder A ← Ch A | `PC4` — GPIO/EXTI (pull-up) | reuse; native 3.3 V |
| Encoder B ← Ch B | `PC5` — GPIO/EXTI (pull-up) | reuse |
| Velocity timebase | `TIM2` (32-bit) free-run | edge timestamps (T/M-method) |
| Current sense (INA238 #6349) | `I2C3` — SCL `PA8` / SDA `PC9` | Adafruit INA238, **onboard 15 mΩ inline** in the M1 lead (no external shunt); @ **0x40** (touch=0x41 idle); reuse board's I2C3 pull-ups, ≤400 kHz; **firmware ADCRANGE=0** (±163.84 mV, ≤10 A) |
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
