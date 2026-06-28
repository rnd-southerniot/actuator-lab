# Commissioning Workflow — the universal bench-test pipeline

Every actuator in this library earns its catalog status by passing this **phase-gated**
procedure. It generalizes the iSV57T bring-up (the worked example in
[actuators/isv57t-servo](../actuators/isv57t-servo)). The rules:

- **Gates are hard.** Do not advance to phase N+1 until phase N's gate passes.
- **Record every gate** in the motor's `COMMISSIONING-LOG.md` (check / expected /
  measured / pass?).
- **Catalog status = highest phase fully passed** (see [CONVENTIONS.md](CONVENTIONS.md)).
- **Motion phases (3–7) need the written safety go-ahead** and the motor secured, no
  load, supply current-limited. Phases 0–2 are setup/read-only/first-energize.
- **Stop immediately** on any unexpected behavior: alarm, runaway, growl/vibration,
  current spike, heat, smell. Power down (motor supply first), diagnose, then resume.

---

## Phase 0 — Bench safety setup  *(read-only)*
Set up the rig before any power.
- Motor body clamped; shaft **unloaded**; rotation path clear; fingers clear.
- PSU **current-limited** (start low, e.g. ~1 A) and **fused**; voltage set to the
  lowest valid supply for first tests.
- Polarity of supply **metered at the connector**.
- Discharge-wait rule noted (per drive manual) for any rewiring.

**Gate:** rig mechanically safe, PSU limited/fused, polarity verified. ☐

## Phase 1 — Controller smoke test  *(read-only)*
Prove the controller + toolchain independently of the motor.
- Upload the smoke firmware (Arduino `blink` / ESP-IDF console boot) with the motor
  **unpowered or control unwired**.
- Confirm serial/console is alive (heartbeat / REPL prompt).

**Gate:** controller flashes and its serial/console responds. ☐

## Phase 2 — Power-on & comms sanity  *(first energize)*
- Apply motor supply in the correct **power-up order** (controller idle first).
- Watch idle current (small, steady), drive **PWR LED** normal, **ALM/fault clear**.
- Read parameters via the vendor tool / bus (record model, firmware, key params,
  alarm map) into `COMMISSIONING-LOG.md`.
- Hand-rotate (power off if needed) to confirm encoder/feedback reads sanely.

**Gate:** no alarm, idle current normal, parameters read, feedback sane. ☐

## Phase 3 — First motion (SAFE-IDLE)  *(motion — needs go-ahead)*
- Firmware boots to a **defined idle** (no motion) and moves only on explicit command.
- Command the **smallest** counted move at the **lowest** rate.
- Verify a small, defined move; flip direction and verify reversal. No alarm.

**Gate:** smallest commanded move observed, both directions, no fault. ☐

## Phase 4 — Scaling / calibration  *(motion)*
- Establish the **unit constant** (pulses-per-rev, counts-per-position, etc.).
- Send a known command; predict the motion (`N command-units ÷ unit = expected`);
  measure against a shaft mark. Test linearity across at least 2–3 magnitudes.

**Gate:** predicted motion = measured (e.g. full-rev mark lands home); constant recorded. ☐

## Phase 5 — Speed survey  *(motion)*
- Step the rate/velocity up in stages across the intended operating band.
- Watch for resonance, roughness, lost sync, alarms at each stage.

**Gate:** smooth across the intended band — or the usable band + limits documented. ☐

## Phase 6 — Repeatability  *(motion)*
- Run **N out-and-back cycles** (e.g. 10×) at a representative speed.
- Check the shaft mark returns home (no drift) and **zero alarm events** across the run.

**Gate:** returns home with no drift, 0 faults over N cycles. ☐

## Phase 7 — Fault handling & recovery  *(motion — needs go-ahead)*
Prove the drive **detects** a fault, **fails safe**, and **recovers cleanly** — the field
behaviour that matters when something jams.
- With the drive enabled/commanded, **induce a fault** by a safe, controlled method —
  e.g. **lock the shaft with power off, hands clear**, then command a slow move into the
  lock → over-current / following-error. (Never grab a moving shaft.)
- Verify the drive **flags** it (alarm/status register set, fault LED) and **stops/holds
  safely** — no runaway, no silent step-loss masquerading as normal.
- **Clear the fault** via the drive's reset (e.g. restart-driver bit / `mot-reset`); confirm
  the alarm register returns to normal.
- Verify **healthy operation resumes** — a normal small move completes with no error/alarm.

**Gate:** fault detected → safe stop → clean reset → normal motion restored → **✅ validated**. ☐

> If the drive is **open-loop** and a stall raises **no** alarm (silent step-loss), record
> that as the finding — the recovery is re-homing, and the lack of stall protection is a
> documented limitation, not a pass of *detection*.

---

## After Phase 7
- Write `RESULTS.md` (the constant, speed band, repeatability, fault-recovery evidence).
- Write `docs/NEXT_SESSION.md` (what's left — loaded testing, tuning, etc.).
- Flip the actuator's catalog row to ✅ in the top-level `README.md`.

> **Grandfathering:** motors validated before Phase 7 was added (iSV57T, IG28ET) keep their
> ✅ — re-run Phase 7 opportunistically when next on the bench.

> Not every motor uses pulse/dir. For Modbus/CAN/analog drives, the *phases stay the
> same* — only the mechanism changes (a "move" is a register write / velocity command,
> the "unit constant" is position counts, etc.). Pick the matching
> [test-harness](../templates/test-harness) as your Phase 1–7 starting firmware.
