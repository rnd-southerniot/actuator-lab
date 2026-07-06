# NEXT SESSION — MG513/GMR + DBH-12V + NUCLEO-F401RE

Handoff note. Written **2026-07-06** (onboarding — actuator-lab entry created, no fresh bench run yet).

## Where we are
| # | Item | State |
|---|---|---|
| 1 | actuator-lab docs (SPECS/WIRING/SAFETY/LOG) from source repo | ✅ |
| 2 | Firmware/host/dashboard | ✅ in submodule [`src/`](../src/) (`stm32-nucleo-gmr-encoder`, pinned) |
| 3 | Phase 1 — serial comms | ✅ per src (5/5, 2026-04-05) — re-confirm on this bench |
| 4 | Phase 0/2 (safety, encoder) | ⏳ src Phase 2 = wiring in progress |
| 5 | Phase 3–7 (motion → faults) | ⛔ needs wiring + 12 V + written go-ahead |

## Exact next steps
1. **Sync the submodule:** `git submodule update --init --recursive` (first checkout). To pull upstream
   fixes later: `git submodule update --remote actuators/mg513-gmr-nucleo-f401/src` then commit the bump.
2. **Wire** per [../WIRING.md](../WIRING.md) — critical: **encoder VCC = 5 V (CN6-5)**, common ground,
   **12 V connected LAST**, SB13/14 ON / SB62/63 OPEN, add the PB1 battery-divider.
3. **Phase 1 re-confirm (no 12 V):** flash `src/firmware`, run `src/host` server / `hw_test --phases 1,2`;
   verify VCP comms + encoder counts by hand.
4. **Phase 3+ need a written go-ahead.** Then: first motion (swap MOTOR A/B if reversed) → **Phase 4
   verify 60 000 cnt/rev** against a shaft mark → RPM survey/auto-tune → position repeatability → faults.

## Gotchas (carried in)
- **Do NOT edit the firmware here** — it's a submodule; fix in `rnd-southerniot/stm32-nucleo-gmr-encoder`
  and bump the pointer.
- Encoder 5 V (not 3.3). PA4 (CT) ≤ 3.3 V; CT floats without 12 V (overcurrent gated on EN).
- 18650 sags under load — a stiff bench PSU is better for any current/characterization work.
- **Don't confuse with [mg513p30-hall-motor](../../mg513p30-hall-motor/)** (Hall 13-PPR, F429+MC33886).

## One-line resume
```bash
cd /Users/robotics/Developer/projects/robotics/actuator-lab && git submodule update --init --recursive && \
  $EDITOR actuators/mg513-gmr-nucleo-f401/WIRING.md actuators/mg513-gmr-nucleo-f401/COMMISSIONING-LOG.md
```
