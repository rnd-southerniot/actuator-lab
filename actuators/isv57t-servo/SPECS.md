# SPECS — Leisai/Leadshine iSV5709V36T-01-1000 (iSV57T-090)  (canonical)

> All values verified from the manual and confirmed on the bench 2026-06-28.
> **Source:** Leadshine iSV57T(S) Full Manual / ISV57T-090 datasheet (OMC-StepperOnline &
> oyostepper mirrors; the Leisai PDF is image-only). Leisai iSV57T V3.0.

## Identity
- `iSV5709V36T-01-1000` = iSV57T(S) series — **57 mm / NEMA 23**, **090 = 90 W**,
  **V36 = 36 V**, integrated BLDC servo with built-in driver + **1000-line encoder
  (= 4000 counts/rev)**.

## Power
- Input: **24–36 V recommended** (absolute 20–50 V). Continuous current **6.0 A max**.
- Back-EMF: manual notes "leave reservation for voltage fluctuation and back-EMF during
  deceleration." No braking resistor needed at no-load.

## Control interface (PUL / DIR)
- Method: single-pulse **PUL + DIR** (rising-edge; edge & mode software-configurable
  Pr0.06/Pr0.07/Pr0.08). Double-pulse CW/CCW also supported.
- Opto-isolated inputs rated **5–24 V directly, 7–16 mA internally current-limited →
  NO external series resistor** (5 V recommended). HIGH 4.5–24 V, LOW 0–0.5 V.
- Timing: min pulse width **2.5 µs** (200 kHz max) or **1 µs** (500 kHz max); **DIR leads
  PUL ≥ 5 µs**. Pulse input range **0–300 kHz**.
- ESP32 (3.3 V) note: below the 4.5 V HIGH window → needs a **5 V buffer** (then no R).

## Feedback / alarm
- Encoder: **1000 lines = 4000 counts/rev**.
- **ALM±:** OC output, **sink/source 50 mA @ 24 V max**. **Normal = low-impedance →
  reads LOW (OK); alarm = high-impedance → reads HIGH** (with pull-up). Active impedance
  software-configurable. It is an OUTPUT — never drive it.

## Comms / tuning port (5V / TX / GND / RX / NC)
- **True RS-232 (±12 V), NOT TTL, NOT isolated.** Pin 1 `+5V` is an OUTPUT — **never
  connect to a PC/MCU**. Pin2 TxD, Pin3 GND, Pin4 RxD, Pin5 NC.
- Use vendor `CABLE-PC-i(PJ)` + USB-RS232 adapter + **isolated supply**. Software: **ACHSeries**.

## DIP switches  (set with power OFF)
| Switch | Function | First-test setting |
|---|---|---|
| S1 S2 S3 | pulses/rev (Off/Off/Off=software Pr0.08; **Off/Off/On = 4000**; On/On/On=8000) | **Off Off On = 4000** |
| S4 S5 | stiffness (Off/Off = software Pr0.03 default 70; On/On = 70) | Off Off |
| S6 | direction (Off = CCW, On = CW) | Off |

> Manual inconsistency: pin-table text says "SW5 reverses direction" but the DIP table
> says **S6** — trust the silkscreen.

## Verified-on-bench
- Arduino Uno, common-anode/active-LOW, direct (no R). See [RESULTS.md](RESULTS.md) and
  [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md). 4000 pulses = exactly 1.000 rev.
