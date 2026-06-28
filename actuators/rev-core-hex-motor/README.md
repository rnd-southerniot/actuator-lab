# REV Core Hex Motor (REV-41-1300)

12 V brushed DC gearmotor (72:1, 5 mm hex out) with a built-in magnetic quadrature encoder —
**closed-loop speed + position** via an **MC33886 H-bridge** on an **STM32 Discovery**, with a
MATLAB/Simulink plant model + system-ID workflow.

## Status

| Item | State | Evidence |
|---|---|---|
| Driver / MCU selected | ✅ **Waveshare RPi Motor Driver Board (2× MC33886)** + **STM32F429I-DISC1** | this folder |
| Doc scaffold | ✅ specs/wiring/safety/modeling drafted (2026-06-29) | — |
| Pinouts (motor / encoder) | ✅ confirmed from REV image | [WIRING.md](WIRING.md) |
| F429 pin map (proposed, reuses st-discovery) | ⏳ pin-conflict audit + Waveshare header pins | [WIRING.md](WIRING.md) |
| Bench bring-up (Phases 0–7) | ⛔ not started | — |
| Simulink model + system-ID | ⛔ not started | [docs/MODELING.md](docs/MODELING.md) |

Status legend in [../../docs/CONVENTIONS.md](../../docs/CONVENTIONS.md). **Firmware lives here in
[test/](test/)** as an `st-discovery`-style Makefile project (arm-none-eabi + st-flash), reusing its
proven F429 motor/encoder pins. Current sensing = **TI INA238** (I²C 16-bit monitor).
**⚠️ Remaining before firmware:** (1) **pin-conflict audit** (per st-discovery `AGENTS.md`) for the new
**FS1** input + **I2C3** (shared with the board's touch); (2) **Waveshare RPi-header pin numbers** for
PWMA/IN1/IN2/FS1 + the **IN/PWM truth table** (wiki); (3) **INA238 shunt (~5 mΩ) + I²C address**.

## Quick start

```
1. Read SAFETY-CHECKLIST.md and WIRING.md (confirm the TBDs above first).
2. Build the test/ harness (pwm-hbridge-encoder-stm32) once the board variant is set.
3. Run ../../docs/COMMISSIONING-WORKFLOW.md Phases 0–7, logging COMMISSIONING-LOG.md.
```

## Files
- [SPECS.md](SPECS.md) — verified interface (canonical)
- [WIRING.md](WIRING.md) — MC33886 + STM32 + encoder + current-sense wiring
- [SAFETY-CHECKLIST.md](SAFETY-CHECKLIST.md) — verified facts + per-session gate
- [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md) — phase-gate record (0–7)
- [RESULTS.md](RESULTS.md) — characterization
- [docs/MODELING.md](docs/MODELING.md) — Simulink plant model + system-ID + control-loop plan
- [docs/NEXT_SESSION.md](docs/NEXT_SESSION.md) — resume roadmap
- [test/](test/) — bench firmware (STM32 PWM H-bridge + quadrature)
