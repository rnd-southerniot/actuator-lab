# Bench Wiring Diagram — iSV5709V36T-01-1000 (iSV57T)

**Common-anode (sinking) wiring. Pulse is ACTIVE-LOW: MCU pin LOW = pulse fires.**
Wire only with power OFF + 5-min discharge. VERIFIED & FIELD-TESTED on Uno 2026-06-28.

```
  ✅ VERIFIED: PUL/DIR opto inputs are rated 5–24V directly, 7–16mA internally
     current-limited.  NO external series resistor needed.  5V is recommended.
     (A 5V Arduino connects straight to PUL±/DIR±.)
  ESP32 path still needs a 5V buffer (3.3V is below the input HIGH window) — see B-2.
```

---

## POWER (both setups, identical)
```
   DC PSU 24V              iSV57T POWER CONNECTOR
   (limit ~1A, fused)
   ┌──────────┐
   │   (+) ●───┼───[ FUSE ]──────────────► +Vdc
   │   (−) ●───┼─────────────────────────► GND
   └──────────┘
   First test: 24V, current limit ~1A. Meter-verify polarity at the connector.
```

---

## OPTION A — ACHSeries first (NO control wiring)
```
   PC ── USB ── [USB-to-RS232 adapter] ── CABLE-PC-i(PJ) ──► tuning port (5V/TX/GND/RX/NC)
   PUL / DIR / ALM:  LEFT UNWIRED at this stage.
   ✅ VERIFIED: port is TRUE RS-232 (NOT TTL) and NOT isolated. Pin1 +5V is an OUTPUT —
   NEVER connect it to a PC/MCU. Vendor cable ONLY; use an isolated supply for the drive.
```

---

## OPTION B-1 — ARDUINO (5V)  ★ recommended first MCU ★
```
   ARDUINO (5V logic)                         iSV57T CONTROL CONNECTOR

   +5V  ●───────────────┬────────────────────► PUL+   ┐ common anode
                        └────────────────────► DIR+   ┘ (opto LED supply)

   D9   ●─────────────────────────────────────► PUL-   (direct, no R; sink LOW = pulse active)
   D8   ●─────────────────────────────────────► DIR-   (direct; set BEFORE pulsing, ≥5µs)

   GND  ●───────────────┬────────────────────► input common / signal GND
                        │                        ◄── the ONE common ground
   ALM read-back (input A0, never drive):
   +5V ──[ pull-up ~4.7k ]──┬─────────────────► ALM+        ALM logic (verified):
   A0   ●───────────────────┘                                normal = LOW (OK)
   GND  ●─────────────────────────────────────► ALM-         alarm  = HIGH

   Idle levels in code: PUL=HIGH, DIR=HIGH (opto off).  Pin LOW = active.
   Din = any spare digital-INPUT pin; firstspin.ino uses A0.
```

## OPTION B-2 — ESP32 (3.3V)  needs a 5V buffer — do NOT wire direct
```
   ESP32 (3.3V)      74HCT244 / 74HCT14 (Vcc = 5V)        iSV57T CONTROL

                     +5V ●──────────┬─────────────────────► PUL+  ┐ common
                                    └─────────────────────► DIR+  ┘ anode
   GPIO25 ●──►[ A1   buffer   Y1 ]─────────────────────────► PUL-   (5V out, no R)
   GPIO26 ●──►[ A2   buffer   Y2 ]─────────────────────────► DIR-
   GND   ●──────────────┬───────────────────────────────────► input common
                        └─ buffer GND ─┘   single common ground

   ALM ► level-shift/clamp to 3.3V BEFORE the ESP32 input pin (it's 5V-pulled).

   Why the buffer: ESP32 3.3V is BELOW the input's 4–5V HIGH window → unreliable /
   half-on opto without it. HCT family accepts 3.3V in, outputs clean 5V.
```

---

## DO / DON'T
```
  DO   set DIR and wait ≥5µs before the first pulse
  DO   start with a COUNTED burst (200 pulses @ ~1kHz), never a free-running train
  DO   keep one — and only one — common ground point
  DO   power up Arduino->idle FIRST, then 24V.  Power down: 24V OFF first, Arduino last.
  DO   set DIP switches with power OFF (S1-S3=Off/Off/On = 4000 pulses/rev)
  DON'T use Servo.h / servo.write() / analogWrite — this is STEP+DIR, not RC PWM
  DON'T drive ALM (it is an OUTPUT)
  DON'T connect ESP32 GPIO straight to PUL-/DIR- (use the 5V buffer)
  DON'T hot-plug the control connector with power on
  DON'T open the serial monitor while 24V live if avoidable (auto-resets the Uno)
```

## ACTIVE-LOW LOGIC QUICK REF (common-anode wiring above)
```
   MCU pin   opto       result
   HIGH  →   off    →   idle (no pulse / DIR = one direction)
   LOW   →   on     →   pulse fires / DIR = other direction
```
> Prefer active-HIGH? Re-wire common-cathode: tie PUL-/DIR- to GND, drive PUL+/DIR+ from 5V logic.
> Then invert the idle levels in code. Pick ONE scheme and keep code + wiring consistent.
>
> DIP (set power-off): S1-S3 = pulses/rev (Off/Off/On=4000) · S4-S5 = stiffness (Off/Off=sw default 70) · S6 = dir (Off=CCW/On=CW).
