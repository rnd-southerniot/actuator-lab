# NEXT SESSION — REV Core Hex Motor

Handoff note. Updated **2026-07-01** after the Phase 4/5/7 bench session. Read first.

## Where we are
| # | Item | State |
|---|---|---|
| 1 | Hardware (F429I-DISC1 + Waveshare 2×MC33886 + Adafruit INA238 15 mΩ) | ✅ live, on `/dev/cu.usbmodem1103` |
| 2 | Firmware (`test/`, branch `rev-core-hex-firmware`) | ✅ built & flashed; boots DISARMED/PWM=0 |
| 3 | Phase 0–3 (safety → first motion, both dir) | ✅ |
| 4 | **Phase 4 (calibration)** — 288 cnt = 1.000 output rev | ✅ **bench-verified** (multi-rev: 896 ct = 3 turns+40° → 288.0) |
| 5 | **Phase 5 (speed survey)** | ⏳ low ✅; **mid/high blocked — supply sag FS-trips** |
| 6 | Phase 6 (repeatability) | ⛔ blocked (needs pos loop + stiff supply) |
| 7 | **Phase 7 (faults)** | ⏳ FS-trip fail-safe + recovery proven; stall/over-temp pending |
| 8 | Simulink model + system-ID | ⛔ not started |

## THE blocker (fix this first — unblocks Phases 5, 6, 7)
**Bare 18650 + BMS supply sags under load → MC33886 undervoltage FS trip.** It tripped `vel 288`
(60 RPM) after ~1 s at ~1.2 A / rail ≈ 10 V, and tightened as the cell drained (even `vel 100`
eventually tripped). **Action:**
- Add a **1000–2200 µF electrolytic across VIN** (also softens spin-up inrush), and/or
- Drive from a **stiff bench PSU** current-limited to ~3 A (≤ 4.4 A stall) for the speed survey.

Then re-run **Phase 5 mid/high** (`vel 288`, `vel 576`) and **Phase 6** out-and-back cycles.

## Firmware item — position loop is not production-ready
`MODE_POS` is a **direct position→duty PID**; with this motor's stiction (breakaway ≈180 duty) +
backlash + 288 CPR it has a **±13-count (±16°) deadband** and limit-cycles (see COMMISSIONING-LOG
Phase 4 finding). Before trusting position control (Phase 6):
- **Cascade position → the existing velocity loop** (preferred), or add velocity + stiction feedforward.
- Consider adding a **software over-current trip** off the INA238 (today the *only* fault path is the
  FS pin — a mechanical stall with FS healthy would NOT latch a fault).

## Bench reminders
- **Gains are NOT persisted** — firmware boots all gains to 0. Each session: `arm`, then
  `gainv 2 20` (velocity). Position needs `gainp` tuned per the cascade fix above.
- INA238 current/bus are **unreliable under PWM** (CM dv/dt) — trust a multimeter during drive; the
  readings are good at rest/DC (bus reads ~12.2 V at rest, matches meter).
- Console verbs: `status arm disarm stop reset jog <duty> <ms> vel <cps> pos <counts> gainv gainp`.
  `cps = RPM/60 × 288` (10/60/120 RPM = 48/288/576 cps). Telemetry @115200 8N1 on USART1.
- Power down: **motor 12 V supply OFF first**, then STM32/USB.

## One-line resume
```bash
# Add VIN bulk cap or switch to a bench PSU, then re-run Phase 5 mid/high + Phase 6.
# Serial helper used last session: scratchpad revcon.py (pyserial, 115200) — send/seq/monitor.
cd /Users/robotics/Developer/projects/robotics/actuator-lab/actuators/rev-core-hex-motor && $EDITOR COMMISSIONING-LOG.md RESULTS.md
```
