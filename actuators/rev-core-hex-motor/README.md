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
| Bench bring-up (Phases 0–7) | ⏳ P0–4 ✅ · P5 partial (supply-limited) · P7 FS-path ✅ · P6 pending | [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md) |
| Simulink model + system-ID | ⛔ not started | [docs/MODELING.md](docs/MODELING.md) |

Status legend in [../../docs/CONVENTIONS.md](../../docs/CONVENTIONS.md). **Firmware lives here in
[test/](test/)** as an `st-discovery`-style Makefile project (arm-none-eabi + st-flash), reusing its
proven F429 motor/encoder pins. Current sensing = **TI INA238** (I²C 16-bit monitor).
Waveshare header pins + truth table are now resolved (PWMA=Pin37, M1=Pin38, M2=Pin40; sign-magnitude;
FS1 on a board pad; 5 A/ch).
**⚠️ Bench-confirm before motion:** (1) **FS1 active polarity** (PB7 reads HIGH idle / LOW on a forced
trip); (2) **INA238** = Adafruit #6349 (onboard 15 mΩ, @0x40) reads sane current — firmware set to
**ADCRANGE=0 / SHUNT_CAL=3072**; (3) meter the board's I2C3 pull-ups before adding the module.

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
