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

## Phase 4 — Scaling / calibration  ✅ 2026-07-02 (go-ahead given)
- **Unit constant bench-verified ≈ 1456 counts = 1.000 output rev** — NOT the datasheet's 1560.
- Method = multi-rev accumulation + shaft mark (as with REV's 288):
| Spin | Counts (Δpos) | Operator read | → cnt/rev | Pass? |
|---|---|---|---|---|
| 5-turn | 15020−6977 = **8043** | 5 turns + ~175° = 5.486 rev | **1466** | ✅ |
| 1-turn | 16475−15020 = **1455** | landed **on the reference** (≈1.00 rev) | **1455** | ✅ |
- **Conclusion:** ≈ **1456 = 13 PPR × 4 × 28** → the actual gearbox is **~28:1, not the nominal 30:1**
  (accounts for the ~6.5% gap vs 1560). Firmware `ENC_COUNTS_PER_OUT_REV` updated 1560 → **1456** + reflashed.
- Note: free speed re-derives to ≈ 1456 × 366/60 ≈ 8880 cps (VEL_MAX_CPS=11000 still has headroom).

## Phase 5 — Speed survey  ✅ 2026-07-02 (go-ahead given; gains `gainv 1 10`)
Closed-loop velocity swept low/mid/high — **no FS trips** (after the debounce fix below).
| Setpoint | Actual (vfilt) | Duty | Notes | Pass? |
|---|---|---|---|---|
| low — 1000 cps (~41 RPM) | ~1000 | 300–558 | tracks; some duty hunting | ✅ |
| mid — 4000 cps (~165 RPM) | ~3600–4030 | 450–999 | tracks; saturates on accel | ✅ |
| high — 7500 cps (~309 RPM) | **plateaus ~7150 (~295 RPM)** | **pinned 999** | duty-saturated → 7500 unreachable at 12 V under load | ⚠️ ceiling |
- **Practical band ≈ 20–290 RPM** (up to ~7000 cps before duty saturates). Vendor no-load 366 RPM is
  open-loop at full 12 V; closed-loop caps lower (LPF lag + slight bus sag + load).
- **Velocity estimate is noisy** (raw `vel_cps` ±20–30 % around `vfilt`); gains are a loose first pass.
  Future: retune `gainv`, and improve the low-speed velocity estimator. Running current ≤ ~1.2 A (accel).

### 🔧 Firmware fix — FS-fault debounce (unblocked this phase)
The unfiltered PB7/FS line false-tripped on EMI at speed (edge-triggered EXTI caught glitches). Changed:
the raw FS EXTI **no longer latches**; the **1 kHz poll latches only after FS reads LOW for
`FS_DEBOUNCE_TICKS = 3` consecutive ticks** (`control_isr`). Rejects sub-ms glitches; a real (held-LOW)
MC33886 fault still latches within ~3 ms (the bridge self-protects meanwhile). Survey then ran glitch-free.
A hardware RC filter (1 k + 100 nF on PB7) remains a good belt-and-suspenders add.

## Phase 6 — Repeatability  ⚠️ go-ahead — NOT YET RUN
- Out-and-back position cycles. Note: cascade the position loop first (REV-build lesson) before trusting
  position control on the gear train.

## Phase 7 — Fault handling & recovery  ⚠️ go-ahead — NOT YET RUN
- Induce stall / FS trip; verify fail-safe + recovery. (FS→fail-safe→reset chain already seen incidentally.)
