# RESULTS — MG513P30 12V (Hall)

Characterization from the commissioning run. Keep concise + evidence-backed.

## Summary
- Date / rig: 2026-07-02 / STM32F429I-DISC1 + Waveshare 2×MC33886 + INA238 (15 mΩ), bench PSU.
- Status: ⏳ **Phases 0–3 PASS** (motor + encoder + both directions validated); closed-loop velocity
  stable (first-pass gains). Phases 4–7 pending.

## Firmware (branch rev-core-hex-firmware, `test/`)
- Ported from REV Core Hex. **`ENC_COUNTS_PER_OUT_REV=1560`** (13 PPR × 4 × 30), **`VEL_MAX_CPS=11000`**
  (free speed ≈ 9500 cps), **PWM 1 kHz** (MC33886 enable limit).
- **Quadrature decode negated** (`d = -kQuadLUT[...]`) — this motor's A/B orientation counted opposite
  to drive; without it, +duty → −vel = closed-loop runaway. Now +duty → +pos/+vel. ✅
- Gains boot to 0 — set each session. First stable velocity: `gainv 1 10` (needs a proper tune).

## Calibration
- **Unit constant:** 1560 counts = 1.000 output rev (13 PPR × 4 × 30) — **bench-verify at Phase 4**
  (multi-rev method, as with REV's 288). Not yet measured against a shaft mark.

## Speed band
- First closed-loop run: **~2000 cps (≈77 RPM) held stable** (`vfilt` ±150) — no runaway, <0.5 A, no FS
  trip. Raw vel-est noise ~±500 cps span at 1560 CPR (LPF tracks). Full sweep toward ≈366 RPM / 9500 cps
  = Phase 5.
- **Static breakaway is high: ~55–60 % duty from rest** (25/50 % jogs stalled; 60 % broke away). Once
  moving, runs at low duty. Low-speed starts need the integral to push through this.

## Repeatability
- Pending Phase 6. **Cascade the position loop before trusting position control** (REV-build lesson —
  the direct position PID limit-cycled and likely killed the REV gearbox on hard reversals).

## Power sequencing / notes
- 12 V, PSU current-limit ~3.5 A (≤ stall 3.2 A), fused. STM32 (USB) first, then 12 V. Running draw is
  only ~0.3–0.5 A — no supply-sag FS trips like the REV unit.
- INA238 bus/current unreliable under PWM (reads far below 12 V) — trust a meter under drive; good at rest.

## Not yet done
- Phase 4 calibration, Phase 5 full speed survey + gain tune, Phase 6 repeatability, Phase 7 faults;
  Simulink system-ID.
