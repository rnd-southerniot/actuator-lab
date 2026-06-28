# COMMISSIONING LOG — <MODEL>

Per-phase record for [../../docs/COMMISSIONING-WORKFLOW.md](../../docs/COMMISSIONING-WORKFLOW.md).
Fill measured/pass for each gate. Catalog status = highest phase fully passed.

Date: ______  Operator: ______  Rig/MCU: ______  Firmware: ______

## Phase 0 — Bench safety setup
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Clamp / no load / path clear | safe | | ☐ |
| PSU current-limited + fused | ~1 A, fused | | ☐ |
| Polarity at connector | correct | | ☐ |

## Phase 1 — Controller smoke test
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Firmware uploads | ok | | ☐ |
| Serial/console alive | heartbeat/REPL | | ☐ |

## Phase 2 — Power-on & comms sanity
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Idle current | small, steady | | ☐ |
| PWR LED / ALM LED | on / clear | | ☐ |
| Params read (model, fw, key regs) | recorded below | | ☐ |
| Encoder/feedback hand-check | sane | | ☐ |

Params recorded: <…>

## Phase 3 — First motion (SAFE-IDLE)
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Smallest move fwd | small defined move | | ☐ |
| Reverse | reverses | | ☐ |
| ALM | clear | | ☐ |

## Phase 4 — Scaling / calibration
- Unit constant: `<N command-units ÷ unit = predicted>` → measured: ______
| Magnitude | Predicted | Measured | Pass? |
|---|---|---|---|
| small | | | ☐ |
| mid | | | ☐ |
| full unit (e.g. 1 rev) | lands on mark | | ☐ |

## Phase 5 — Speed survey
| Rate/speed | Smooth? | Notes | Pass? |
|---|---|---|---|
| low | | | ☐ |
| mid | | | ☐ |
| high (band limit) | | | ☐ |

## Phase 6 — Repeatability
| Metric | Expected | Measured | Pass? |
|---|---|---|---|
| N out-and-back cycles | returns home | | ☐ |
| Drift | none | | ☐ |
| Alarm events | 0 | | ☐ |

**Result:** ☐ ✅ validated (Phase 6 passed) — update catalog row.
