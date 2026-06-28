# ACHSeries Parameter-Read Log — iSV5709V36T-01-1000 (iSV57T)

**Step 3 of bench plan: read & record BEFORE any motion command.**
Connection: vendor `CABLE-PC-i(PJ)` + USB-RS232 adapter. Motor at 24 V, current limit ~1 A, PUL/DIR UNWIRED.

Date: ____________  Operator: ____________  ACHSeries version: ____________
Firmware / drive model on label: ____________________________

---

## 1. Comms & boot sanity
| Check | Expected | Actual | Pass? |
|---|---|---|---|
| ACHSeries connects (COM port, baud) | links, reads | __________ | ☐ |
| Drive model / firmware reported | matches label | __________ | ☐ |
| PWR LED | solid/normal | __________ | ☐ |
| ALM LED | off / no alarm | __________ | ☐ |
| Idle supply current | small, steady | _______ A | ☐ |

## 2. Alarm register (decode every active/historical alarm)
| Alarm code | Meaning (per manual) | Active now? | Cleared? |
|---|---|---|---|
| __________ | __________________ | ☐ | ☐ |
| __________ | __________________ | ☐ | ☐ |
| __________ | __________________ | ☐ | ☐ |
> Any active alarm ⇒ do NOT command motion. Resolve first.

## 3. Control / motion parameters (record values — needed to scale §6 pulse test)
| Parameter | ACHSeries name / Pr# | Value | Notes |
|---|---|---|---|
| Control / command mode | __________ | __________ | pulse/dir? |
| Pulse input mode | __________ | __________ | PUL+DIR / CW+CCW / quad |
| Active pulse edge | __________ | __________ | rising? |
| Electronic gear numerator | __________ | __________ | |
| Electronic gear denominator | __________ | __________ | |
| **Command pulses / revolution** | __________ | __________ | **key for §6 scaling** |
| Encoder resolution (counts/rev) | __________ | __________ | expect 4000 |
| Max speed limit | __________ | __________ | rpm |
| Current / torque limit | __________ | __________ | |
| Soft position limits (if any) | __________ | __________ | |
| Direction polarity / invert | __________ | __________ | |
| Alarm output logic | __________ | __________ | matches Checklist D |

## 4. Encoder feedback sanity (power may be OFF for hand-rotation)
| Action | Expected | Actual | Pass? |
|---|---|---|---|
| Read position at rest | stable value | __________ | ☐ |
| Nudge shaft CW by hand | count changes monotonically | __________ | ☐ |
| Nudge shaft CCW | count reverses | __________ | ☐ |

## 5. Built-in Trial Run / Jog (smallest move, lowest speed)
| Test | Setting | Result | Pass? |
|---|---|---|---|
| Smallest jog, low speed, forward | _______ rpm / _______ rev | smooth, no alarm | ☐ |
| Reverse | same | clean reversal | ☐ |
| Waveform monitor: position-following | — | no runaway/oscillation | ☐ |
| Waveform monitor: current | — | within limit, no spikes | ☐ |

---

### GATE — proceed to MCU pulse test (§6 Stage 2) only when:
☐ Comms OK, no active alarms, idle current clean
☐ Command pulses/rev recorded (Section 3) — you will compare to pulses sent
☐ Trial Run spun smoothly both directions with healthy waveform
☐ Pre-Wiring Checklist A–G complete, resistor value chosen

**Computed expectation for first MCU burst:**
200 pulses ÷ (______ pulses/rev) = ______ rev expected at the shaft.
Measure actual rotation against this after the §6 burst.

Notes / anomalies:
________________________________________________________________
________________________________________________________________
