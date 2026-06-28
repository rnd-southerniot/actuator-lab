# <MOTOR NAME> (<MODEL>)

<one-line description: type, frame, voltage, control method>

## Status

| Item | State | Evidence |
|---|---|---|
| Bench bring-up (Phases 0–6) | ⛔ not started | — |
| Vendor-tool trial run | ⛔ | — |
| Loaded testing | ⛔ | — |

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
