# COMMISSIONING LOG — REV Core Hex Motor

Per-phase record for [../../docs/COMMISSIONING-WORKFLOW.md](../../docs/COMMISSIONING-WORKFLOW.md)
(Phases 0–7). Fill measured/pass for each gate. Catalog status = highest phase fully passed.
**Motion phases (3–7) need the written safety go-ahead.**

Date: ______  Operator: ______  Rig/MCU: STM32 Discovery (‹variant›)  Driver: MC33886  Firmware: ____

## Phase 0 — Bench safety setup
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Body clamped / hex output secured / path clear | safe | | ☐ |
| PSU 12 V, current-limited + fused | ~1.5 A, fused | | ☐ |
| Polarity at JST-VH | correct | | ☐ |
| 12 V isolated from STM32/encoder (3.3 V) | isolated | | ☐ |

## Phase 1 — Controller smoke test
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Firmware builds + flashes (STM32) | ok | | ☐ |
| Console/telemetry alive (VCP/SWO) | heartbeat | | ☐ |
| PWM = 0 at boot (idle), EN safe | idle | | ☐ |

## Phase 2 — Power-on & feedback sanity  *(read-only)*
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Idle current | ~0, steady | | ☐ |
| MC33886 FS (fault) | clear | | ☐ |
| Encoder hand-rotate: A/B count up/down, direction sane | clean quadrature | | ☐ |
| Counts-per-output-rev hand-check | ≈ 288 | | ☐ |
| Current-sense ADC reads 0 at rest, scales with hand-load | sane | | ☐ |

Params recorded: encoder CPR=____ FS polarity=____ no-load current=____ ADC scale=____

## Phase 3 — First motion (SAFE-IDLE)  ⚠️ go-ahead
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Smallest PWM step fwd | small defined move | | ☐ |
| Reverse (DIR flip) | reverses, encoder sign flips | | ☐ |
| FS / current | clear, within limit | | ☐ |

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
