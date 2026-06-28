# actuator-lab — execution contract

Point-of-truth library for bench-tested actuators. This file is the standing contract
for any agent or engineer working in this repo. Read it before touching hardware.

## Identity & scope
- Org: `rnd-southerniot`. Owner: Arif, Head of R&D, Southern IoT.
- This repo is a **catalog + per-actuator docs + native test firmware**. It is NOT a
  shared code library — each motor uses its own native toolchain (Arduino / ESP-IDF /
  MATLAB). Standardize the *docs template*, not the platform.

## 🔒 Safety gate (highest priority — overrides convenience)
1. **No motion without a written go-ahead.** Any step that energizes the drive or moves
   the shaft requires explicit confirmation in the session, with the motor secured,
   shaft unloaded/clear, supply current-limited and fused.
2. **Read-only is always safe:** datasheet reads, param reads, bus scans, status,
   `blink`/console boot. These need no gate.
3. **Power sequencing:** controller powered & idle FIRST, then motor supply. Power down
   in reverse (motor supply OFF first). Honor the drive's discharge wait before rewiring.
4. **One change at a time** when debugging hardware; never stack conflicting fixes.
5. Every actuator folder has a `SAFETY-CHECKLIST.md` — it is authoritative for that motor.

## Conventions
- Per-actuator file set (see `templates/_actuator-template/`): `README.md`, `SPECS.md`,
  `WIRING.md`, `SAFETY-CHECKLIST.md`, `COMMISSIONING-LOG.md`, `RESULTS.md`,
  `docs/NEXT_SESSION.md`, `test/`.
- **`SPECS.md` is canonical** for a motor's verified interface facts — cite the manual
  (name + version + date) for every value. Don't trust the catalog over `SPECS.md`.
- Catalog status mirrors the highest commissioning phase passed (see
  `docs/CONVENTIONS.md`).
- Status emojis: ✅ validated · ⏳ partial · ⚠️ blocked · ⛔ not started · 📘 reference.

## Onboarding a new actuator
Test-driven — follow `docs/ADD-A-MOTOR.md`, which runs the phase-gated
`docs/COMMISSIONING-WORKFLOW.md` (Phase 0 bench-safety → Phase 6 repeatability). A motor
is only ✅ when Phase 6 passes, recorded in its `COMMISSIONING-LOG.md`.

## Submodules
- Existing code repos enter as submodules (e.g. `actuators/leesn-ig28et-stepper`,
  `reference/qube-servo2-plant`). They keep their own CI/history.
- Clone with `--recurse-submodules`; update with `git submodule update --remote`.
- Do NOT edit submodule internals from here — fix in the source repo and bump the pointer.

## Toolchain notes
- Arduino targets: `arduino-cli` (AVR core installed). Compile check:
  `arduino-cli compile --fqbn arduino:avr:uno <sketchdir>`.
- ESP-IDF targets live in submodules with their own build instructions.
- Serial monitor on macOS: `arduino-cli monitor` is flaky for capture; use a pyserial
  reader (opening the port auto-resets an Uno).
