# RESULTS — MG513/GMR + DBH-12V + NUCLEO-F401RE

Characterization from the commissioning run. Keep concise + evidence-backed.
Firmware/host in submodule [`src/`](src/); its own metrics live in [src/HARDWARE_STATUS.md](src/HARDWARE_STATUS.md).

## Summary
- Date / rig: 2026-07-06 (onboarding) / NUCLEO-F401RE + DBH-12V + MG513 GMR (30:1).
- Status: ⏳ **onboarding** — docs from source repo; Phase 1 (serial comms) ✅ per src (5/5, 2026-04-05).
  Phases 0/2 pending, 3–7 need wiring + 12 V + go-ahead.

## Calibration
- **Unit constant:** 60 000 counts = 1.000 output rev (500 PPR × 4 × 30) — **bench-verify at Phase 4**
  (sibling MG513P30 gearbox measured 28:1 not 30 — do not assume). Measured: pending.

## Speed band
- Pending Phase 5 (closed-loop RPM; firmware has relay-feedback auto-tune). Free speed: confirm.

## Repeatability
- Pending Phase 6 (position mode; 360° = 1 output rev target).

## Fault handling
- Firmware provides: overcurrent (CT, EN-gated), stall detection, E-STOP/brake, IWDG watchdog,
  low-battery (PB1 divider). Verify at Phase 7.

## Notes / gotchas (from source repo)
- **Encoder needs 5 V** (CN6-5), not 3.3 V. **12 V connected LAST.** **PA4 (CT) ≤ 3.3 V.**
- **HW-BUG-01:** CT floats without 12 V → overcurrent check gated on EN=HIGH (fixed upstream).
- 18650 pack sags under load — prefer a stiff PSU for characterization/current work.

## Not yet done
- Everything past comms: encoder verify, first motion, calibration, RPM survey, position repeatability,
  faults; on-bench re-confirmation of the source repo's Phase-1 result.
