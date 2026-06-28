# COMMISSIONING LOG — iSV57T (iSV5709V36T-01-1000)

Worked reference example — **all six gates passed 2026-06-28** → ✅ validated.
Workflow: [../../docs/COMMISSIONING-WORKFLOW.md](../../docs/COMMISSIONING-WORKFLOW.md).

Date: 2026-06-28  Operator: Arif  Rig/MCU: Arduino Uno @ /dev/cu.usbmodem1101
Firmware: test/firstspin/firstspin.ino  Supply: 24 V, ~1 A limit

## Phase 0 — Bench safety setup
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Clamp / no load / path clear | safe | clamped, no load | ☑ |
| PSU current-limited + fused | ~1 A | 24 V, ~1 A | ☑ |
| Polarity at connector | correct | metered OK | ☑ |
| DIP set power-off | 4000 ppr | S1-S3 Off/Off/On | ☑ |

## Phase 1 — Controller smoke test
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| blink uploads | ok | ok (2050 B) | ☑ |
| Serial alive @115200 | heartbeat | "Uno alive" + LED 1 Hz | ☑ |

## Phase 2 — Power-on & comms sanity
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Idle current | small, steady | steady idle | ☑ |
| ALM (via A0) | LOW = OK | LOW, stable ×3 | ☑ |
| Drive PWR / ALM LED | on / clear | normal / clear | ☑ |

Params: pulses/rev set by DIP (4000). ACHSeries vendor trial-run **skipped** (went
straight to MCU path — see appendix [docs/ACHSERIES-PARAM-LOG.md](docs/ACHSERIES-PARAM-LOG.md)).

## Phase 3 — First motion (SAFE-IDLE)
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| 200-pulse fwd (`f`) | ~18° (0.05 rev) | shaft stepped, clean | ☑ |
| Reverse (`r`) | reverses | reversed same amount | ☑ |
| ALM | clear | LOW throughout | ☑ |

## Phase 4 — Scaling / calibration
- Unit constant: `4000 pulses ÷ 4000 ppr = 1.000 rev`.
| Magnitude | Predicted | Measured | Pass? |
|---|---|---|---|
| 200 | 0.05 rev (~18°) | matched | ☑ |
| 800 | 0.20 rev (~72°) | 4× the 200 move | ☑ |
| 4000 | 1.000 rev | mark landed home | ☑ |

## Phase 5 — Speed survey
| Rate | rpm | Smooth? | Pass? |
|---|---|---|---|
| ~1 kHz | 15 | smooth | ☑ |
| ~2 kHz | 30 | smooth | ☑ |
| ~5 kHz | 75 | smooth (bit-bang jitter limit beyond) | ☑ |

## Phase 6 — Repeatability
| Metric | Expected | Measured | Pass? |
|---|---|---|---|
| 10× (fwd+rev 1 rev) @ 30 rpm | returns home | no drift | ☑ |
| Total pulses | — | 80,000 | — |
| Alarm events | 0 | 0 | ☑ |

**Result:** ☑ ✅ validated — Phase 6 passed. Catalog row = ✅.
