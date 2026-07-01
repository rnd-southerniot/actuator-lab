# MG513P30 12V (Hall encoder)

12 V brushed DC gearmotor, **30:1**, with a 13-PPR magnetic **Hall** quadrature encoder — closed-loop
speed + position via an **MC33886 H-bridge** on an **STM32F429I-DISC1**. Replaces the retired REV Core
Hex (gearbox failure) on the same rig; firmware ported from that build.

## Status

| Item | State | Evidence |
|---|---|---|
| Specs confirmed (vendor sheet) | ✅ | [SPECS.md](SPECS.md) — 13 PPR, 30:1, 366 RPM, 0.36/3.2 A |
| Firmware ported + compiles | ✅ | `test/` (CPR 1560, VEL clamp 11000, decode negated); `make` clean |
| Bench bring-up | ⏳ **Phases 0–3 PASS** | motor+encoder+both dirs OK; closed-loop vel stable ([COMMISSIONING-LOG](COMMISSIONING-LOG.md)) |
| Phase 4–7 | ⛔ | calibration, speed survey, repeatability, faults pending |

Status legend in [../../docs/CONVENTIONS.md](../../docs/CONVENTIONS.md).

## Quick start

```
1. Read SAFETY-CHECKLIST.md and WIRING.md.
2. Set the test/ harness config block to match SPECS.md.
3. Run docs/../../../docs/COMMISSIONING-WORKFLOW.md Phases 0–6, logging COMMISSIONING-LOG.md.
```

## Files
- [SPECS.md](SPECS.md) — verified interface (canonical)
- [WIRING.md](WIRING.md) — pinout, power, control wiring
- [SAFETY-CHECKLIST.md](SAFETY-CHECKLIST.md) — verified facts + per-session gate
- [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md) — phase-gate record
- [RESULTS.md](RESULTS.md) — characterization
- [docs/NEXT_SESSION.md](docs/NEXT_SESSION.md) — resume roadmap
- [test/](test/) — bench firmware
