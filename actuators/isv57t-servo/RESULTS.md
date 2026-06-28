# RESULTS — iSV57T (iSV5709V36T-01-1000)

## Summary
- Date / rig: 2026-06-28, Arduino Uno @ /dev/cu.usbmodem1101, 24 V no-load.
- Status: ✅ validated (Phases 0–6).

## Calibration
- **Unit constant: 4000 pulses = 1.000 rev** (DIP S1-S3 = Off/Off/On), verified against a
  shaft mark — full revolution landed exactly home.
- Linearity: confirmed across 200 / 800 / 4000 pulses (0.05 / 0.20 / 1.000 rev).

## Speed band
- Tested 15 / 30 / 75 rpm (≈1 / 2 / 5 kHz) — all smooth, ALM clear.
- Limit: above ~75 rpm the **bit-banged** `digitalWrite` timing gets jittery; for higher
  speed move to a hardware timer (Uno Timer1) or ESP32 RMT/MCPWM with an accel ramp.

## Repeatability
- 10× out-and-back (1 rev each) @ 30 rpm = **80,000 pulses, 0 alarm events, no drift**.

## Power sequencing / notes
- Power-up: Uno booted to idle FIRST, then 24 V. Power-down: 24 V first, Uno last.
- Wiring: opto inputs 5–24 V internally limited → **direct, no series resistor**.
- ALM normal = LOW; opening the serial port auto-resets the Uno (brief boot float).

## Not yet done
- ACHSeries vendor trial-run + following-error/stiffness (S4/S5) tuning.
- Higher-speed via hardware-timer pulse gen with accel ramp.
- Loaded testing (re-check current, thermals, following error under inertia).
