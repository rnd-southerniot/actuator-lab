# MG513 / GMR encoder — NUCLEO-F401RE + DBH-12V

MG513 brushed DC gearmotor (30:1) with a **GMR 500-PPR** quadrature encoder, driven by a **DBH-12V**
H-bridge from an **STM32 NUCLEO-F401RE**. Closed-loop RPM + cascaded position control, relay-feedback
auto-tune, COBS/CRC-16 telemetry @921600, and a host server + web dashboard.

**Firmware, host, and dashboard live in the submodule [`src/`](src/)** = `rnd-southerniot/stm32-nucleo-gmr-encoder`
(vendored, unmodified — clone with `--recurse-submodules`; fix upstream and bump the pointer, never edit here).
This actuator-lab folder holds the **phase-gated commissioning docs** for the same hardware.

## Status

| Item | State | Evidence |
|---|---|---|
| Specs / wiring / safety (from src) | ✅ | [SPECS.md](SPECS.md) · [WIRING.md](WIRING.md) · [SAFETY-CHECKLIST.md](SAFETY-CHECKLIST.md) |
| Phase 1 — controller/serial comms | ✅ per src (5/5, 2026-04-05) | [src/HARDWARE_STATUS.md](src/HARDWARE_STATUS.md); re-confirm on this bench |
| Phase 0, 2 (safety, encoder) | ⏳ pending | src Phase 2 wiring in progress |
| Phase 3–7 (motion → faults) | ⛔ go-ahead | needs wiring + 12 V + written go-ahead |

Status legend: [../../docs/CONVENTIONS.md](../../docs/CONVENTIONS.md).

## Quick start
1. Read [SAFETY-CHECKLIST.md](SAFETY-CHECKLIST.md) + [WIRING.md](WIRING.md) (esp. **encoder 5 V**, **12 V last**).
2. Firmware/host are in `src/` — build/flash per [src/README.md](src/README.md) (`cd src/firmware && make`;
   OpenOCD flash). Host: `cd src/host && python server.py --port <VCP> --baud 921600`.
3. Run [docs/COMMISSIONING-WORKFLOW.md](../../docs/COMMISSIONING-WORKFLOW.md) Phases 0–7, logging
   [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md). Motion phases (3–7) need a written go-ahead.

## Files
- [SPECS.md](SPECS.md) · [WIRING.md](WIRING.md) · [SAFETY-CHECKLIST.md](SAFETY-CHECKLIST.md) · [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md) · [RESULTS.md](RESULTS.md) · [docs/NEXT_SESSION.md](docs/NEXT_SESSION.md)
- [src/](src/) — the `stm32-nucleo-gmr-encoder` submodule (firmware + host + dashboard + its own docs)
