# Add a Motor — test-driven onboarding pipeline

Onboarding an actuator is a **test process**. You don't mark a motor "validated" because
its docs are filled in — you mark it validated because it **passed the gates** in
[COMMISSIONING-WORKFLOW.md](COMMISSIONING-WORKFLOW.md). This page is the checklist that
ties the template, the test harness, and the workflow together.

## Steps

1. **Copy the template.**
   ```bash
   cp -r templates/_actuator-template actuators/<motor-slug>
   ```
   Use a clear slug, e.g. `isv57t-servo`, `leesn-ig28et-stepper`, `gm5208-foc`.

2. **Fill specs & safety from the manual** *(read-only desk work).*
   - `SPECS.md` — every interface fact, each **cited** to the manual (name/version/date).
   - `SAFETY-CHECKLIST.md` — verified facts (resistor/buffer needs, signal levels, ALM
     logic, supply range) + the per-session mechanical gate.
   - `WIRING.md` — connector pinout, power, control-signal options, DO/DON'T.

3. **Drop in the matching test harness.**
   ```bash
   cp -r templates/test-harness/<class>/* actuators/<motor-slug>/test/
   ```
   Pick by control class:
   - `pulse-dir-arduino/` — step/direction motors on Arduino (SAFE-IDLE counted bursts).
   - `modbus-rs485-espidf/` — Modbus RTU drives on ESP32 (gated console: scan/read/jog/goto).

   Then edit the harness **config block** (pins, rate, burst, active-level / bus params)
   to match `SPECS.md`. Compile-check before touching hardware:
   ```bash
   arduino-cli compile --fqbn arduino:avr:uno actuators/<motor-slug>/test/firstspin   # pulse-dir
   ```

4. **Run the commissioning workflow, Phases 0–6**, recording each gate in
   `COMMISSIONING-LOG.md`. Motion phases (3–6) need the written safety go-ahead.

5. **Write up results.**
   - `RESULTS.md` — unit constant, speed band, repeatability evidence.
   - `docs/NEXT_SESSION.md` — what's left (loaded test, tuning, …).

6. **Publish.** Add/flip the actuator's row in the top-level
   [README catalog](../README.md); status = highest phase passed
   ([CONVENTIONS.md](CONVENTIONS.md)).

## If it's an existing repo (not a fresh bench build)
Add it as a **submodule** instead of copying code:
```bash
git submodule add <repo-url> actuators/<motor-slug>
```
Then add its catalog row pointing into the submodule's own `docs/`. The submodule keeps
its native layout; you don't re-template it.

## Worked example
[actuators/isv57t-servo](../actuators/isv57t-servo) is a complete pass — read its
`COMMISSIONING-LOG.md` to see all six gates filled, and its `test/firstspin` is the
source the `pulse-dir-arduino` harness was generalized from.
