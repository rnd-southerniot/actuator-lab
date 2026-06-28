# SAFETY CHECKLIST — <MODEL>

Two parts: **A–F verified facts** (fill once from the manual) and **G per-session gate**
(re-check every bench session). Don't wire control until A–F are filled.

Manual ref: <name / version / date>.

## A. Control-input drive requirement
- Logic levels HIGH/LOW: <…>; series resistor or buffer? <…>; isolated? <…>. ☐

## B. Comms / tuning port level
- Type (RS-232 / TTL / USB), isolated?, any pin that must NOT touch a PC/MCU. ☐

## C. Signal timing (pulse drives)
- Min pulse width, DIR setup, max freq, active edge. ☐

## D. Alarm / fault output
- Type + rating; **active logic** (normal=?, alarm=?); read-back pull-up plan. ☐

## E. Feedback / resolution
- Encoder counts-per-rev; command unit constant; config-switch setting. ☐

## F. Power supply
- Range, continuous/peak current; first-test PSU setting (V, current limit, fuse). ☐

## G. Per-session mechanical/safety gate  *(every session)*
- ☐ Motor clamped, shaft unloaded, rotation path & fingers clear
- ☐ PSU current-limited + fused; polarity metered at the connector
- ☐ Config switches set with power OFF
- ☐ Power-up order: controller idle FIRST, then motor supply
- ☐ Discharge-wait acknowledged before any rewiring

### GATE
☐ A–F verified from manual (no guesses) · ☐ G re-checked this session · ☐ written
go-ahead before any motion phase. If any blank → stop; resolve from manual / vendor.
