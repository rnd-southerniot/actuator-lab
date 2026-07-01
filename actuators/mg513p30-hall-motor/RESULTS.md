# RESULTS — MG513P30 12V (Hall)

Characterization from the commissioning run. Keep concise + evidence-backed.

## Summary
- Date / rig: 2026-07-02 / STM32F429I-DISC1 + Waveshare 2×MC33886 + INA238 (15 mΩ), bench PSU.
- Status: ✅ **VALIDATED — all gates 0–7 passed** (2026-07-02). Motion, calibration, speed survey,
  repeatability, and fault handling all confirmed on hardware.

## Firmware (branch rev-core-hex-firmware, `test/`)
- Ported from REV Core Hex. **`ENC_COUNTS_PER_OUT_REV=1560`** (13 PPR × 4 × 30), **`VEL_MAX_CPS=11000`**
  (free speed ≈ 9500 cps), **PWM 1 kHz** (MC33886 enable limit).
- **Quadrature decode negated** (`d = -kQuadLUT[...]`) — this motor's A/B orientation counted opposite
  to drive; without it, +duty → −vel = closed-loop runaway. Now +duty → +pos/+vel. ✅
- Gains boot to 0 — set each session. First stable velocity: `gainv 1 10` (needs a proper tune).

## Calibration ✅ (Phase 4, 2026-07-02)
- **Unit constant ≈ 1456 counts = 1.000 output rev** — bench-measured (1455 over 1 rev landing on-mark;
  1466 over 5 revs). **NOT the datasheet 1560** — actual gearbox is **~28:1, not 30:1** (13 PPR × 4 × 28).
  Firmware `ENC_COUNTS_PER_OUT_REV = 1456`.

## Speed band ✅ (Phase 5, 2026-07-02; `gainv 1 10`)
- Swept 1000 / 4000 / 7500 cps: low (~41 RPM) & mid (~165 RPM) track; **high 7500 (309 RPM)
  duty-saturates at 999 and plateaus ~7150 cps (~295 RPM)**. **Practical band ≈ 20–290 RPM.**
- Vendor no-load 366 RPM is open-loop @ full 12 V; closed-loop caps lower (LPF lag + bus sag + load).
- **Velocity estimate noisy** (raw ±20–30 % around `vfilt`); gains loose first pass — retune + better
  low-speed estimator = future work. Current ≤ ~1.2 A on accel; **no FS trips** (see debounce below).
- **Static breakaway is high: ~55–60 % duty from rest** (25/50 % jogs stalled; 60 % broke away). Once
  moving, runs at low duty. Low-speed starts need the integral to push through this.
- **FS-fault debounce added** (firmware): raw PB7 EXTI no longer latches; the 1 kHz poll latches only
  after FS LOW for 3 consecutive ticks — rejects EMI glitches that false-tripped at speed. Real
  (held-LOW) faults still latch in ~3 ms. Optional HW belt-and-suspenders: 1 k + 100 nF RC on PB7.

## Repeatability ✅ (Phase 6, 2026-07-02)
- 5 out-and-back cycles of 1 rev: returns home to **728 ±1 count**, no accumulating drift, 0 faults.
- Position control is a **cascade** (position-P → clamped velocity setpoint → inner velocity PI); a
  728-count move decelerates smoothly and parks in a 12-count deadband — **no overshoot, no limit-cycle**
  (unlike the REV direct-duty PID). `gainp` = position-P gain in cps/count (`gainp 3 0 0` used).

## Fault handling ✅ (Phase 7, 2026-07-02)
- **FS-fault** (PB7): debounced — latches only after LOW for 3 consecutive 1 kHz ticks (rejects EMI
  glitches that false-tripped at speed); real held-LOW faults latch in ~3 ms.
- **Stall detector** (encoder-based): |duty| ≥ 850 with < 80 counts of motion over 500 ms → FAULT.
  Position-windowed (robust to hand-stall jitter). Demonstrated: held jog → FAULT → PWM 0 → reset →
  healthy motion. No false-trips on normal low/mid/high starts or position moves.
- A raw stall is otherwise NOT self-reported (MC33886 didn't FS-trip at 45 % held; INA238 unusable under
  PWM for current sensing) — the detector is the guard.

## Power sequencing / notes
- 12 V, PSU current-limit ~3.5 A (≤ stall 3.2 A), fused. STM32 (USB) first, then 12 V. Running draw is
  only ~0.3–0.5 A — no supply-sag FS trips like the REV unit.
- INA238 bus/current unreliable under PWM (reads far below 12 V) — trust a meter under drive; good at rest.

## Not yet done
- Phase 4 calibration, Phase 5 full speed survey + gain tune, Phase 6 repeatability, Phase 7 faults;
  Simulink system-ID.
