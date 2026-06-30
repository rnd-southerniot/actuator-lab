# RESULTS — REV Core Hex Motor

Characterization from the commissioning run. Fill from
[COMMISSIONING-LOG.md](COMMISSIONING-LOG.md) as phases pass; model results in
[docs/MODELING.md](docs/MODELING.md).

## Summary
- Date / rig: 2026-07-01 / STM32F429I-DISC1 + Waveshare 2× MC33886 + Adafruit INA238 (15 mΩ)
- Status: ⏳ Phases 0–3 PASS; open- & closed-loop velocity working (Phase 5 in progress)

## Tuned control gains (firmware `test/`, branch rev-core-hex-firmware)
- **PWM: 1 kHz** (NOT 20 kHz — MC33886 enable limit). Duty range 0–999 (PWM_ARR).
- **Velocity loop (`gainv kp ki`): kp=2, ki=20** → stable, well-damped; `vfilt` holds setpoint within
  ~2 cps, duty steady (e.g. 80 cps → duty ≈ 204). dt-scaled integral + term-clamp anti-windup.
- **Velocity feedback:** raw T-method clamped to ±700 cps, EMA-filtered (VEL_LPF_ALPHA=0.10, ~16 Hz).
  Essential — raw low-CPR velocity is too noisy to close a loop on (caused violent limit cycle).
- Position loop (`gainp kp ki kd`): not yet tuned.
- Open-loop: ~250 duty (25%) → ~110 cps; breakaway needs the proper 1 kHz drive.

## Calibration
- **Unit constant:** 288 counts = 1.000 output rev (vendor) — bench-verify at Phase 4.
- Linearity: pending.

## Speed band
- Tested: pending Phase 5 (closed-loop velocity, input-capture T/M).
- Recommended operating band: pending (free speed 125 RPM output).

## Repeatability
- Pending Phase 6 (note gearbox backlash in the drift budget).

## Identified model parameters (Simulink system-ID — see MODELING.md)
- Ra, La, Ke, Kt, J, b, gear 72:1, backlash: pending.

## Power sequencing / notes
- 12 V, current-limit ≥ stall 4.4 A only deliberately; STM32 (USB) first, then 12 V.

## Not yet done
- Everything: bench bring-up, system-ID, loop tuning, loaded testing.
