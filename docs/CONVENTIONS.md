# Conventions

## Status emojis (catalog + per-motor README)
Status = the **highest commissioning phase fully passed**
(see [COMMISSIONING-WORKFLOW.md](COMMISSIONING-WORKFLOW.md)).

| Emoji | Meaning | Maps to |
|---|---|---|
| ⛔ | not started | no phases run |
| ⏳ | partial | Phases 0–N passed, more remain |
| ⚠️ | blocked | stuck at a gate (note the blocker) |
| ✅ | validated | Phase 6 passed |
| 📘 | reference | modeling/design only, not a driven actuator |

## Per-actuator file set (native folders)
Every driven motor folder under `actuators/` follows
[templates/_actuator-template/](../templates/_actuator-template):

| File | Purpose |
|---|---|
| `README.md` | overview + status table + quick start |
| `SPECS.md` | **canonical** verified interface facts, each cited to the manual |
| `WIRING.md` | connector pinout, power, control options, DO/DON'T, power-up order |
| `SAFETY-CHECKLIST.md` | verified facts (A–G) + per-session mechanical gate |
| `COMMISSIONING-LOG.md` | per-phase (0–6) record sheet: check / expected / measured / pass? |
| `RESULTS.md` | characterization: unit constant, speed band, repeatability |
| `docs/NEXT_SESSION.md` | resume roadmap (where-we-are table, next steps, gotchas, one-line resume) |
| `test/` | native bench firmware (from `templates/test-harness/`) |

Submodule-backed motors keep their own layout; they're summarized by their catalog row,
which links into the submodule's own docs.

## Source-of-truth rules
- `SPECS.md` wins over the catalog if they ever disagree — fix the catalog.
- Cite the manual (name + version + date) for every spec value.
- The catalog `README.md` table is the index; it points to folders/submodules.

## Naming
- Actuator slug: lowercase, hyphenated, model-first — `isv57t-servo`,
  `leesn-ig28et-stepper`, `gm5208-foc`.
- Test harness folders named by **control class**, not by motor:
  `pulse-dir-arduino`, `modbus-rs485-espidf`.

## Formatting
- ASCII schematics and code in fenced blocks with a language tag.
- Tables pipe-delimited. Checkboxes `☐`/`☑` for gates.
- Keep verified values and their manual citation together.
