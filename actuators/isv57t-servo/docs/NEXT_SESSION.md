# NEXT SESSION — iSV57T

Handoff written 2026-06-28. Bench bring-up is complete (Phases 0–6 ✅). What remains is
tuning/production, not basics.

## Where we are
| # | Item | State |
|---|---|---|
| 1 | Bench bring-up (Phases 0–6) | ✅ validated |
| 2 | ACHSeries vendor trial-run + stiffness tuning | ⏳ pending (skipped) |
| 3 | Higher-speed via hardware-timer pulse gen | ⛔ not started |
| 4 | Loaded testing | ⛔ not started |

## Exact next steps
### Step A — ACHSeries trial-run (low-risk now)
Connect vendor `CABLE-PC-i(PJ)` + USB-RS232 (isolated supply), run ACHSeries trial-run,
view live following-error waveform, tune stiffness S4/S5 (or Pr0.03). Fill the appendix
[docs/ACHSERIES-PARAM-LOG.md](ACHSERIES-PARAM-LOG.md).

### Step B — Hardware-timer pulses
Replace bit-bang with Uno Timer1 (or ESP32 RMT/MCPWM) + trapezoidal accel to exceed
~75 rpm jitter-free.

### Step C — Loaded testing
Couple to the mechanism; re-check current, thermals, following error under inertia.

## Gotchas (already cost time)
- Opto inputs are 5–24 V internally limited → **no series resistor** (5 V Arduino direct).
- Tuning port is **true RS-232, not TTL, not isolated**; pin1 +5V must NOT touch a PC/MCU.
- ALM normal = LOW; alarm = HIGH (pull-up). Opening serial **auto-resets the Uno**.
- DIP set with power OFF; manual mislabels direction as SW5 vs S6 — trust silkscreen.

## One-line resume
```bash
arduino-cli compile --fqbn arduino:avr:uno test/firstspin && \
arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn arduino:avr:uno test/firstspin
```
