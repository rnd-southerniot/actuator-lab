# WIRING — REV Core Hex Motor (MC33886 + STM32 Discovery)

Wire only with power OFF + discharge wait. Confirm every value against [SPECS.md](SPECS.md).
Board: **STM32F429I-DISC1** (F429ZI). Encoder/motor pinouts confirmed from the REV image.
**⚠️ Remaining TBDs:** exact F429 **timer/pin assignment** (avoid LCD-SPI5 / L3GD20 / USB pins) and
the **MC33886 module pinout + FS polarity**. Placeholders marked `‹…›`.

## POWER
```
   DC PSU 12 V            MC33886                       REV Core Hex (2-pin JST-VH)
   (limit ~1.5 A, fused)
     (+) ──[ FUSE ]──────► VPWR
     (−) ─────────────┬──► PGND
                      │
   STM32 3V3 ─────────┼──► VDD (logic)        OUT1 ─────► M+  (JST-VH)
   STM32 GND ─────────┴──► AGND ── common ──  OUT2 ─────► M-  (JST-VH)
   First test: 12 V, current limit ~1.5 A (motor stall 4.4 A — start limited), fused.
   Meter-verify polarity at the JST-VH before connecting.
```

## CONTROL — Waveshare RPi Motor Driver Board (2× MC33886), Motor-A channel
Driven from the F429, **not** a Pi. The board's logic passes through a 74LVC8T245 (3.3↔5 V), so feed
the RPi-40-pin header positions at 3.3 V and supply 3V3 to the board (translator VCA reference).
Reuses the proven `st-discovery` DBH-12V mapping (PWM + IN1 + IN2 + encoder) — see that repo's
`docs/stm32f429i-disc1-pin-mapping.md`.
```
   STM32F429 (3.3 V)              Waveshare header        → MC33886 U3 (Motor A: M1/M2)
   PB4  TIM3_CH1 (~20 kHz) ─────► PWMA  ‹header pin›  ─245─► PWM  (chops the bridge)
   PA5  GPIO out          ─────► IN1   ‹header pin›  ─245─► IN1  (direction)
   PA7  GPIO out          ─────► IN2   ‹header pin›  ─245─► IN2  (direction)
   ‹FT pin› GPIO in       ◄───── FS1   ‹header pin›  (open-drain, 1k pull-up to **5 V** — use a
                                                       5 V-tolerant pin or divide; trip PWM→0 on fault)
   3V3  ───────────────────────► 3.3V  header pin     (74LVC8T245 VCA ref)
   GND  ───────────────────────► GND   header pin     ◄── single common ground
   12 V PSU ───────────────────► VIN (Power conn.)    board buck makes its own 5 V; motor via MC33886
```
> IN1/IN2 set direction; **PWMA chops the output** — confirm the exact IN/PWM truth table + the
> RPi-header pin numbers for PWMA/IN1/IN2/FS1 from the Waveshare wiki before wiring. The 74LVC8T245
> DIR/OE are hardwired Pi-as-master (we drive the inputs — matches). ⚠️ Don't sustain stall (board
> ensured 3 A < 4.4 A stall).

## CURRENT SENSE — TI INA238 (I²C power monitor, external)
```
   board OUT1 (M+) ──►[ Rshunt ~5 mΩ ]──► motor M+     (in-line; INA238 reads SIGNED current)
                        │            │
                  INA238 IN+      INA238 IN-
                  INA238 VBUS ──► 12 V node            (bus-voltage readout, for V·I param-ID)
                  INA238 SCL/SDA ─── I²C ───► STM32 I2C3 (SCL PA8, SDA PC9) + 2.2–4.7k pull-ups to 3V3
                  INA238 A0/A1 ─── set address (avoid the on-board STMPE811 touch @ ~0x41/0x44)
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
| Motor PWM → board PWMA | `PB4` — TIM3_CH1 (AF2) | reuse (DBH map); ~20 kHz |
| Direction → board IN1 | `PA5` — GPIO out | reuse |
| Direction → board IN2 | `PA7` — GPIO out | reuse |
| Fault ← board FS1 | `‹free FT pin›` GPIO in | board pulls FS to **5 V** → use a 5 V-tolerant pin or a divider; audit |
| Encoder A ← Ch A | `PC4` — GPIO/EXTI (pull-up) | reuse; native 3.3 V |
| Encoder B ← Ch B | `PC5` — GPIO/EXTI (pull-up) | reuse |
| Velocity timebase | `TIM2` (32-bit) free-run | edge timestamps (T/M-method) |
| Current sense (INA238) | `I2C3` — SCL `PA8` / SDA `PC9` | shared w/ on-board STMPE811 touch — set INA238 addr to not collide; 2.2–4.7k pull-ups. (PC3 now free) |
| Control ISR | `TIM6`/`TIM7` (basic) @ 1 kHz | PID loop |
| UART telemetry | `PA9`/`PA10` — USART1 | reuse; 115200 8N1 |
| LCD telemetry (optional) | LTDC (board-reserved) | reuse st-discovery LCD-plot for live traces |

> Already proven free in st-discovery: PB4, PA5, PA7, PC4, PC5, PA9, PA10. To audit: the **FS1** input
> (5 V-tolerant) + **I2C3** (PA8/PC9 are the board's STMPE811 touch bus — coexist via a non-colliding
> INA238 address, or drop touch). Per st-discovery `AGENTS.md`, document the final map in the firmware README.
