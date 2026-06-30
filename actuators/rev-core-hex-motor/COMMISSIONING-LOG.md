# COMMISSIONING LOG — REV Core Hex Motor

Per-phase record for [../../docs/COMMISSIONING-WORKFLOW.md](../../docs/COMMISSIONING-WORKFLOW.md)
(Phases 0–7). Fill measured/pass for each gate. Catalog status = highest phase fully passed.
**Motion phases (3–7) need the written safety go-ahead.**

Date: 2026-06-30  Operator: Arif  Rig/MCU: STM32F429I-DISC1  Driver: MC33886 (Waveshare RPi Motor Driver Board)  Firmware: Steps 1–4, branch `rev-core-hex-firmware` @ 884591b
Toolchain: Arm GNU 15.2.rel1 (vendored) · st-flash 1.8.0 · telemetry USART1 115200 via ST-LINK VCP (/dev/cu.usbmodem1103)

## Phase 0 — Bench safety setup
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Body clamped / hex output secured / path clear | safe | | ☐ |
| PSU 12 V, current-limited + fused | ~1.5 A, fused | | ☐ |
| Polarity at JST-VH | correct | | ☐ |
| 12 V isolated from STM32/encoder (3.3 V) | isolated | | ☐ |

## Phase 1 — Controller smoke test  ✅ 2026-06-30
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Firmware builds + flashes (STM32) | ok | `st-flash ... verified, jolly good`; chipid 0x419, 2 MB | ✅ |
| Console/telemetry alive (VCP/SWO) | heartbeat | STATUS @ 10 Hz on USART1; green LED ~5 Hz heartbeat | ✅ |
| PWM = 0 at boot (idle), EN safe | idle | boots `state=DISARMED→FAULT`, `mode=IDLE`, `duty=0` | ✅ |

## Phase 2 — Power-on & feedback sanity  *(read-only)*  ✅ 2026-06-30
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Idle current | ~0, steady | `cur_ma=0` at rest, DISARMED, 12 V bus up | ✅ |
| MC33886 FS (fault) | clear | `fs=1` once board 5 V up; `reset` cleared FAULT→DISARMED | ✅ |
| Encoder hand-rotate: A/B count up/down, direction sane | clean quadrature | CW: 0→294 (+vel); CCW: 294→7 (−vel); monotonic, no jumps | ✅ |
| Counts-per-output-rev hand-check | ≈ 288 | ~294 counts / 1 hand-rev (+2%, hand imprecision) → confirms 288 | ✅ |
| Current-sense reads 0 at rest, scales with load | sane | reads 0 at rest; bus `12330 mV` matches ~12.3 V PSU. Load-scale → Phase 3 (needs drive) | ✅* |

Params recorded: encoder CPR≈**288** (294 meas/hand-rev) · FS polarity=**active-LOW** (LOW=fault, HIGH=ok) · no-load current=**0 mA** (DISARMED) · INA238 bus scale **verified** (12.33 V); current-LSB scaling to be confirmed under drive at Phase 3.
Round-trip: 0→294→7 (residual 7 cnt ≈ 9°, hand imprecision — no systematic edge loss).
Console RX confirmed (VCP bidirectional): `reset` → `OK reset`.

**Phases 0–2 PASS. ⛔ STOP — Phase 3 (first commanded motion) requires written go-ahead.**

## Phase 3 — First motion (SAFE-IDLE)  ✅ 2026-07-01 (go-ahead given; motor body-clamped, shaft free)
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Smallest PWM step fwd | small defined move | `jog 250` (25% @ 1 kHz): pos 0→61 smooth, vel ~+110 cps, cur ~300–550 mA | ✅ |
| Reverse (DIR flip) | reverses, encoder sign flips | `jog -250`: pos 65→4, vel ~−105 cps, **signed current −630 mA**, fs clear | ✅ |
| FS / current | clear, within limit | fs=1 throughout; current sane; bus held ~8–10.5 V (no brownout) | ✅ |

### ⚠️ ROOT-CAUSE FINDINGS (cost ~a session of debugging — read before next bring-up)
1. **PWM frequency: PWMA drives the MC33886 *enable*, which CANNOT switch at 20 kHz.** At 20 kHz the bridge
   never enabled → both outputs floated to +12 V → zero drive. Waveshare's own demo PWMs it at **500 Hz**;
   firmware changed **20 kHz → 1 kHz** (TIM3 prescaler) and the motor drove instantly. (SPECS.md had flagged
   the 20 kHz as "confirm against MC33886 limits" — it was wrong.)
2. **VCCA (3V3→header Pin 1) must be a SOLID connection.** A flaky jumper left VCCA floating <1 V → the
   74LVC8T245 translator passed control signals only intermittently. Re-seated → steady 2.917 V → fixed.
   (Board 3V3/VCCA runs fine at ~2.9 V; the Discovery 3V3 sags under LCD+peripheral load but stays in spec.)
3. **INA238 current AND bus readings are UNRELIABLE during PWM** (shunt on a PWM'd lead, high CM dv/dt — the
   WIRING.md caveat). It reported bus "6.5 V" while a meter showed VIN steady at 12.5 V; current read low.
   **Use a multimeter for ground truth during drive.** Readings are trustworthy at rest / DC.
4. Direction convention: **+duty → +pos (forward), −duty → −pos**; INA238 returns signed current (− in reverse).
5. Tooling: telemetry + console both work over the **ST-LINK VCP** (/dev/cu.usbmodem1103) — RX confirmed.
   Note: ~25% duty draws ~0.5 A and the 18650+BMS rail dips to ~8–10 V (no bulk cap yet) — fine, but a
   ~1000–2200 µF cap across VIN would steady it and soften spin-up inrush (one inrush fault seen at 100%).

## Phase 4 — Scaling / calibration  ⚠️ go-ahead
- Unit constant: **288 counts = 1.000 output rev** → measured against an output-shaft mark: ______
| Magnitude | Predicted | Measured | Pass? |
|---|---|---|---|
| small (e.g. 72 counts = 90°) | | | ☐ |
| mid (144 counts = 180°) | | | ☐ |
| full rev (288 counts) | lands on mark | | ☐ |

## Phase 5 — Speed survey  ⚠️ go-ahead
- Closed-loop velocity (input-capture T/M-method). Sweep the band toward 125 RPM free speed.
| Speed (RPM out) | Smooth? | Vel-est noise | Pass? |
|---|---|---|---|
| low (~10) | | | ☐ |
| mid (~60) | | | ☐ |
| high (~120, band limit) | | | ☐ |

## Phase 6 — Repeatability  ⚠️ go-ahead
| Metric | Expected | Measured | Pass? |
|---|---|---|---|
| N out-and-back position cycles | returns home | | ☐ |
| Drift (account for backlash) | bounded | | ☐ |
| Fault events | 0 | | ☐ |

## Phase 7 — Fault handling & recovery  ⚠️ go-ahead
- Induce a fault (locked-output stall and/or MC33886 over-current/over-temp); verify detection +
  fail-safe + recovery.
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Drive/firmware flags fault | FS asserts and/or current-limit trips PWM→0 | | ☐ |
| Fails safe | motor stops, no runaway | | ☐ |
| Reset/recover | clear cause → re-enable → fault clears | | ☐ |
| Healthy motion resumes | small move, no fault | | ☐ |

**Result:** ☐ ✅ validated (Phase 7 passed) — update catalog row.
