# test-harness: pwm-hbridge-encoder-stm32

Starting point for **brushed DC servos** driven by a **PWM H-bridge** with a **quadrature encoder**,
on **STM32** (HAL). Use this when a "move" is a PWM duty + DIR (not step/dir pulses, not a fieldbus
register write) and feedback is an incremental encoder. First user:
[`actuators/rev-core-hex-motor`](../../../actuators/rev-core-hex-motor) (MC33886 + STM32 Discovery).

## Why a new class
The existing harnesses don't fit a PWM brushed DC servo:
- `pulse-dir-arduino` → step/direction stepper drives.
- `modbus-rs485-espidf` → integrated drives with a register bus.

A brushed DC servo needs **PWM + DIR + an H-bridge + encoder decode + a control loop on the MCU** —
the loop lives in *our* firmware, not in a smart drive.

## Building blocks (STM32 HAL)
| Concern | STM32 mechanism |
|---|---|
| Motor PWM (~20 kHz) | `TIMx` PWM output → H-bridge IN/PWM |
| Direction / enable | GPIO → H-bridge IN/EN; drive to safe state at boot (**PWM = 0 idle**) |
| Fault read-back | GPIO input ← driver fault flag (e.g. MC33886 FS); trip PWM→0 on fault |
| Position | `TIMn` **encoder mode** (A/B) |
| Velocity (low-CPR) | **input capture** edge timestamps → T/M-method (don't use counts/sample at low speed) |
| Current | analog: `ADC` ← shunt + INA amp · OR digital: I²C power monitor (e.g. **INA238** — also gives bus-V + temp). Protection + torque loop + system-ID |
| Control loop | `TIMk` 1 kHz ISR → cascade current/velocity/position PID with anti-windup |
| Console | USB-VCP / UART gated REPL: telemetry (read-only) + jog/goto/vel (motion, gated) |

## Console pattern (map onto the workflow)
| command | phase | safe? |
|---|---|---|
| `read` / `status` (pos, vel, current, FS) | P2 | ✅ read-only |
| `jog <duty> <ms>` (bounded, auto-stop) | P3/P5 | ⚠️ motion |
| `goto <counts>` (position loop, capped) | P4/P6 | ⚠️ motion |
| `fault` / `reset` (induce/clear, recovery) | P7 | ⚠️ motion |

## To onboard a PWM-H-bridge DC servo
1. Copy this skeleton into `actuators/<slug>/test/`; set the **pin/timer + PWM-freq + encoder-CPR +
   current-scale** config block from the motor's `SPECS.md` / `WIRING.md`.
2. Confirm the **MCU variant** (timer availability, 5 V-tolerant pins) and the **driver fault polarity**.
3. Boot to **PWM = 0**; verify encoder direction + CPR by hand (P2) before any motion.
4. Run Phases 0–7, logging `COMMISSIONING-LOG.md`. Model/tune in Simulink (see the motor's
   `docs/MODELING.md`).

> Watch-outs: low-CPR encoders make **velocity** the hard part (edge-timing, not counts/sample);
> gearbox **backlash** caps position repeatability; size the **H-bridge for stall current** and trip
> on the fault flag. Motion writes are physical — gate them.
