# RESULTS — REV Core Hex Motor

Characterization from the commissioning run. Fill from
[COMMISSIONING-LOG.md](COMMISSIONING-LOG.md) as phases pass; model results in
[docs/MODELING.md](docs/MODELING.md).

## Summary
- Date / rig: 2026-07-01 / STM32F429I-DISC1 + Waveshare 2× MC33886 + Adafruit INA238 (15 mΩ)
- Status: ⏳ Phases 0–4 PASS; Phase 5 PARTIAL (low-speed validated, mid/high **supply-limited**);
  Phase 7 PARTIAL (FS-trip fail-safe + recovery proven). **Blocker: stiff VIN supply (bulk cap / PSU).**

## Tuned control gains (firmware `test/`, branch rev-core-hex-firmware)
- **PWM: 1 kHz** (NOT 20 kHz — MC33886 enable limit). Duty range 0–999 (PWM_ARR).
- **Velocity loop (`gainv kp ki`): kp=2, ki=20** → stable, well-damped; `vfilt` holds setpoint within
  ~2 cps, duty steady (e.g. 80 cps → duty ≈ 204). dt-scaled integral + term-clamp anti-windup.
- **Velocity feedback:** raw T-method clamped to ±700 cps, EMA-filtered (VEL_LPF_ALPHA=0.10, ~16 Hz).
  Essential — raw low-CPR velocity is too noisy to close a loop on (caused violent limit cycle).
- **Position loop (`gainp kp ki kd`): characterized, NOT production-ready.** Direct position→duty PID
  has a **~±13-count (±16°) deadband** from stiction (breakaway ≈180 duty/18%) + backlash. `kp=4,ki=0`
  stalls short; adding integral overshoots and limit-cycles. Best gentle compromise tried: `kp=2,ki=6`.
  **Fix before relying on position control:** cascade position→velocity, or add velocity/stiction
  feedforward. **NB gains are NOT persisted in firmware** — re-apply `gainv 2 20` (and any `gainp`) each
  session; they boot to 0.
- Open-loop: ~250 duty (25%) → ~110 cps; breakaway needs the proper 1 kHz drive.

## Calibration
- **Unit constant: 288 counts = 1.000 output rev — ✅ bench-verified (Phase 4, 2026-07-01).**
  Multi-rev method: 896-count spin = operator-read 3 turns + ~40° = 3.111 rev → **288.0 cnt/rev**
  (±0.9%, ≈±3 ct). Confirms vendor value.
- Linearity: pending.

## Speed band
- **Low ~10 RPM (`vel 48`): ✅** smooth, vfilt holds 47 cps, raw vel-est noise ±3 cps (span 6).
- **Mid ~60 RPM (`vel 288`): controller OK** (reached & held 288 cps ~1 s) but **FS-trips** — bare
  18650+BMS sags to ~10 V at ~1.2 A → MC33886 undervoltage. Trip threshold worsens as the cell drains.
- **High ~120 RPM: blocked** by the same supply limit (not attempted).
- Recommended operating band: **pending a stiff supply** (free speed ~125 RPM / ~600 cps). Re-run after
  adding a **1000–2200 µF VIN cap** and/or a bench PSU.

## Repeatability
- Pending Phase 6 (note gearbox backlash in the drift budget).

## Identified model parameters (Simulink system-ID — see MODELING.md)
- Ra, La, Ke, Kt, J, b, gear 72:1, backlash: pending.

## Power sequencing / notes
- 12 V, current-limit ≥ stall 4.4 A only deliberately; STM32 (USB) first, then 12 V.

## Not yet done
- Everything: bench bring-up, system-ID, loop tuning, loaded testing.
