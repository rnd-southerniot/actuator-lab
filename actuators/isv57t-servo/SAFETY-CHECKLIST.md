# Pre-Wiring Verification Checklist — iSV5709V36T-01-1000 (iSV57T)

**Rule: Do not connect a single control wire until every item below is filled in from YOUR manual.**
Power off + 5-minute discharge for all wiring work. Vendor RS232 cable only for tuning port.

Manual ref: Leisai iSV57T (V3.0) / iSV57T(S) full manual (ISV57T-090 variant).
**Values below VERIFIED from manual + field-tested on Arduino Uno 2026-06-28.**

---

## A. Opto-input series resistor  ✅ VERIFIED
- Inputs rated **5–24 V directly, 7–16 mA internally current-limited → NO external resistor needed.**
- **5 V is recommended.** A 5 V Arduino connects straight to PUL±/DIR±. (Field-tested: direct, works.)
- Input type: opto-isolated. HIGH = **4.5–24 V**, LOW = **0–0.5 V**, ~10 mA typ.
- ESP32 exception: 3.3 V < 4.5 V HIGH window → **needs a 5 V buffer** (then also no resistor).

## B. Tuning-port signal level  ✅ VERIFIED
- Port `5V/TX/GND/RX/NC` is **TRUE RS-232 (±12 V), NOT TTL, and NOT isolated.**
- Pin1 `+5V` is an **OUTPUT — never connect to a PC/MCU.** Pin2 TxD, Pin3 GND, Pin4 RxD, Pin5 NC.
- Use vendor `CABLE-PC-i(PJ)` + USB-RS232 adapter + **isolated supply** for the drive. ACHSeries software.

## C. Pulse / direction timing  ✅ VERIFIED
- Min pulse width **2.5 µs** (200 kHz max) or **1 µs** (500 kHz max). DIR leads PUL by **≥5 µs**.
- Edge & max-freq software-configurable (Pr0.07/Pr0.08). Pulse input range **0–300 kHz**.
- Field-tested clean at 1–5 kHz (15–75 rpm) bit-banged, no accel ramp.

## D. ALM alarm output  ✅ VERIFIED  (OUTPUT — never drive it)
- Type: **OC output, sink/source 50 mA @ 24 V max.** Active impedance software-configurable.
- Logic: **normal = low-impedance → reads LOW (OK); alarm = high-impedance → reads HIGH** (with pull-up).
- Read-back: 4.7 kΩ pull-up to +5 V → Arduino **A0** (firstspin.ino). ESP32: level-shift/clamp to 3.3 V.

## E. Electronic gear / resolution  ✅ VERIFIED
- Encoder **1000 lines = 4000 counts/rev**. Field-confirmed: 4000 pulses = exactly 1.000 rev.
- DIP **S1-S3 = pulses/rev** (Off/Off/On = 4000 in use). Software default cmd mode Pr0.08.
- Input mode: single-pulse **PUL + DIR** (rising-edge, software-configurable).

## F. Power supply  ✅ VERIFIED
- Range: **24–36 V recommended** (absolute 20–50 V). Continuous current **6.0 A max**.
- **First-test PSU setting:** 24 V, current limit ~1 A, inline fuse ☐
- Back-EMF/regen: manual notes "leave reservation for voltage fluctuation and back-EMF during decel."
  No braking resistor at no-load; reconsider under load / aggressive decel.

## G. Mechanical / safety
- Motor body clamp-able, no load, rotation path clear ☐
- 5-minute discharge wait acknowledged ☐
- Polarity of `+Vdc / GND` meter-verified at connector ☐

---

### GATE — A–F are VERIFIED (✅). Per-session checks still required:
☑ A–F verified from manual + field-tested 2026-06-28 (no resistor; RS-232 not TTL; ALM LOW=OK; 4000 pulses/rev; 24–36 V)
☐ **G mechanical/safety re-checked THIS session** (clamp, no load, polarity metered, discharge)
☐ DIP switches set with power OFF (S1-S3=Off/Off/On=4000, S4-S5=Off/Off, S6=Off)
☐ Power-up order: Arduino→idle FIRST, then 24 V

Open question for next session: ACHSeries trial-run was skipped — do it before loaded testing. Support: tech@leadshine.com.
