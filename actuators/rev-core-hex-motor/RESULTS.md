# RESULTS — REV Core Hex Motor

Characterization from the commissioning run. **Pending — no bench data yet** (Phases 0–7 not started).
Fill from [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md) as phases pass; model results in
[docs/MODELING.md](docs/MODELING.md).

## Summary
- Date / rig: — / STM32 Discovery (‹variant›) + MC33886
- Status: ⛔ not started

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
