# COMMISSIONING LOG — MG513P30 12V (Hall)

Per-phase record for [../../docs/COMMISSIONING-WORKFLOW.md](../../docs/COMMISSIONING-WORKFLOW.md).
Catalog status = highest phase fully passed.

Date: 2026-07-02  Operator: Arif  Rig/MCU: STM32F429I-DISC1 + Waveshare 2×MC33886 + INA238 (15 mΩ)
Firmware: `mg513p30-f429i` (branch rev-core-hex-firmware; ported from REV Core Hex build)

## Phase 0 — Bench safety setup  ✅ 2026-07-02
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Clamp / no load / shaft clear | safe | motor secured, shaft free | ✅ |
| PSU current-limited + fused | ≤ stall 3.2 A | bench PSU limited ~3.5 A, fused | ✅ |
| Polarity at connector | correct | motor pair → M1; direction corrected in fw (see Phase 3) | ✅ |

## Phase 1 — Controller smoke test  ✅ 2026-07-02
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Firmware uploads | ok | `make flash` verified (st-flash) | ✅ |
| Serial/console alive | heartbeat/REPL | USART1 115200; STATUS stream + verbs OK | ✅ |

## Phase 2 — Power-on & feedback sanity  *(read-only)*  ✅ 2026-07-02
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Idle current | small, steady | 0 mA at rest; bus 12.3 V (matches PSU) | ✅ |
| FS / fault | clear | `fs=1` at rest; DISARMED boot, PWM=0 | ✅ |
| Encoder hand/jog-check | counts, sane | counts cleanly, high-res (≈1560 CPR territory; verify @P4) | ✅ |

### ⚠️ Bring-up findings (read before next session)
1. **Motor/encoder harness was initially mis-landed** → no spin + no counts. After re-seating (2 motor
   leads = few-Ω coil → M1; 4 thin encoder leads → VCC 3.3 V / GND / A→PC4 / B→PC5), both work.
2. **High static breakaway:** the motor does **not** start from rest below ~**55–60 % duty** (25 %/50 %
   jogs stalled at ~0.12–0.47 A; `jog 600` broke away and spun freely). Once moving it runs down to low
   duty fine. The velocity loop's integral must ramp through this to start — watch low-speed starts.
3. **Direction was inverted** (+duty → −pos). Fixed in firmware (negated quadrature decode) so
   +duty → +pos/+vel — required for closed-loop stability. Verified both ways (Phase 3).
4. PWM confirmed **1 kHz** (MC33886 enable limit — carried from REV; correct, not a motor property).

## Phase 3 — First motion (SAFE-IDLE)  ✅ 2026-07-02 (go-ahead given; clamped, shaft free)
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Forward (+duty) | small defined move, +pos | `jog 600`: pos 0→607, vel +618→+1362 cps, ~0.4 A | ✅ |
| Reverse (−duty) | reverses, −pos | `jog -600`: pos 607→−67, vel −1074→−2666, signed cur − | ✅ |
| FS / current | clear, sane | `fs=1` throughout; ~0.3–0.5 A; no trip | ✅ |

### Closed-loop velocity — first stable run (Phase 5 preview)  ⏳ 2026-07-02
- `gainv 1 10`, `vel 2000` (≈77 RPM out): **stable**, `vfilt` held ~2000 ±150 cps for 3.5 s, duty
  modulated 190–665, current <0.5 A, no runaway, `fs=1`. **Loop sign correct.** Raw vel noisy (~±500 cps
  span) but the LPF tracks. Gains are a first pass — full tune + speed survey = Phase 5.

## Phase 4 — Scaling / calibration  ⚠️ go-ahead — NOT YET RUN
- Unit constant to verify: **1560 counts = 1.000 output rev** (13 PPR × 4 × 30) — multi-rev method
  (as with REV's 288). Measured: ______

## Phase 5 — Speed survey  ⚠️ go-ahead — partial (see preview above)
- Sweep toward free speed ≈ 366 RPM (≈ 9500 cps). Tune `gainv`; log smoothness + vel-est noise.

## Phase 6 — Repeatability  ⚠️ go-ahead — NOT YET RUN
- Out-and-back position cycles. Note: cascade the position loop first (REV-build lesson) before trusting
  position control on the gear train.

## Phase 7 — Fault handling & recovery  ⚠️ go-ahead — NOT YET RUN
- Induce stall / FS trip; verify fail-safe + recovery. (FS→fail-safe→reset chain already seen incidentally.)
