# iSV57T — Leisai/Leadshine iSV5709V36T-01-1000

Integrated **BLDC servo**, NEMA 23 / 90 W / 36 V, **pulse-dir** control, 4000 counts/rev.
Bench-validated on Arduino Uno 2026-06-28 — the **worked reference** for this library.

## Status

| Item | State | Evidence |
|---|---|---|
| Bench bring-up (Phases 0–6) | ✅ validated | [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md): 4000 pulses = 1.000 rev, 80k-pulse repeatability, 0 ALM |
| ACHSeries vendor trial-run | ⏳ pending | skipped — went straight to MCU path |
| Loaded testing | ⛔ not started | — |

## Quick start (Arduino Uno)
```bash
arduino-cli compile --fqbn arduino:avr:uno test/firstspin
arduino-cli upload  -p /dev/cu.usbmodem1101 --fqbn arduino:avr:uno test/firstspin
# serial @115200: i=status  f/r=counted burst (MOVES)  +/- rate  ]/[ count
```
Wiring (verified, no resistor): `+5V→PUL+/DIR+`, `D9→PUL-`, `D8→DIR-`, `GND→common`,
`ALM+→4.7k pull-up→A0`. DIP power-off: S1-S3 Off/Off/On (4000 ppr), S4-S5 Off/Off, S6 Off.

## Files
- [SPECS.md](SPECS.md) — canonical verified interface
- [WIRING.md](WIRING.md) — schematics + DO/DON'T
- [SAFETY-CHECKLIST.md](SAFETY-CHECKLIST.md) — verified facts + per-session gate
- [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md) — all six gates (worked example)
- [RESULTS.md](RESULTS.md) — characterization
- [docs/NEXT_SESSION.md](docs/NEXT_SESSION.md) — resume roadmap
- [docs/ACHSERIES-PARAM-LOG.md](docs/ACHSERIES-PARAM-LOG.md) — vendor-tool param sheet (appendix)
- [test/](test/) — `firstspin` (bench driver) + `blink` (smoke)
