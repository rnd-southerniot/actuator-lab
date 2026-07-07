# WIRING — MG513P30 12V (Hall encoder)

Wire only with power OFF + discharge wait. Confirm every value against [SPECS.md](SPECS.md).

**Same rig as the (retired) REV Core Hex build:** STM32F429I-DISC1 + Waveshare 2× MC33886 board +
Adafruit INA238. Only the **motor + encoder leads** change. Reuse the REV wiring for PWM/DIR/FS/I2C
verbatim — the pin map below is carried over and unchanged.

## Motor + encoder — MG513P30 6-lead harness
> Lead **colours vary between batches — verify with a meter before trusting them.** Motor leads =
> the two heavy wires (read a few ohms across them, coil). Encoder leads = the four thin wires.
> **Encoder VCC = 3.3 V** (SPECS: range 3.3–5 V; pick 3.3 V so A/B swing 0–3.3 V and are native-safe on
> PC4/PC5 — no divider). Hall output is push-pull, internally pulled to VCC → no external pull-up.

| MG513 lead | Connect to | Notes |
|---|---|---|
| Motor + (heavy) | MC33886 **M1** output screw terminal | polarity sets direction sign — swap if `+duty → −pos` |
| Motor − (heavy) | MC33886 **M1** other output | (INA238 15 mΩ is inline in this M1 lead) |
| Encoder **VCC** | STM32 **3.3 V** | NOT 5 V — keeps A/B at 3.3 V for PC4/PC5 |
| Encoder **GND** | common **GND** | single common ground with the board |
| Encoder **A** | STM32 **PC4** (EXTI) | native 3.3 V |
| Encoder **B** | STM32 **PC5** (EXTI) | native 3.3 V |

## Carried over from the REV rig (unchanged)
| Signal | STM32 pin | To board | Note |
|---|---|---|---|
| PWM (speed) | `PB4` → PWMA | Waveshare **Pin 37** | TIM3_CH1 **1 kHz** (MC33886 enable limit — NOT 20 kHz) |
| Direction A | `PA5` → M1 | **Pin 38** | sign-magnitude (M1/M2 pair) |
| Direction B | `PA7` → M2 | **Pin 40** | sign-magnitude |
| MC33886 **FS1** | `PB7` (in, FT) | board FS1 pad | open-drain, **1k→5 V**, active-LOW → PWM=0 & latch |
| VCCA (logic ref) | 3.3 V | board **Pin 1** | **must be solid** (Phase-3 lesson: flaky VCCA = intermittent control) |
| GND | GND | Pin 34/39 | common |
| INA238 SCL/SDA | `PA8`/`PC9` (I2C3) | INA238 @0x40 | reuse board I2C3 pull-ups, ≤400 kHz |
| **Motor supply** | — | board **VIN** screw terminal | **12 V**; current-limit **~3.5 A** (stall 3.2 A), fused |

## DO / DON'T
- **DO** power the encoder at **3.3 V**, not 5 V.
- **DO** set the bench PSU current limit **~3.5 A** (above the 3.2 A stall so it won't false-fold, but
  fused). Running current is only ~0.36 A, so normal drive is gentle. *(The REV unit's mystery ~1 A
  trips turned out to be a damaged gearbox, not the supply — a healthy MG513 draws far less.)*
- **DON'T** wire the board's 5 V header pin to the F429 (it's USB-powered) — power-select switch OFF.
- **DON'T** energize the motor supply before the STM32 is powered and DISARMED (power sequencing).

## Power-up / down order
1. STM32 (USB) first → boots **DISARMED**, PWM = 0. 2. Then motor 12 V supply.
Down: motor supply OFF first, then STM32.

## Bench checks before motion (Phase 0–2)
- Encoder: hand-rotate output shaft, confirm `pos` counts up one way / down the other, ~**1560 counts/rev**
  (verify exactly at Phase 4).
- FS: PB7 HIGH idle / LOW on a forced trip; `fs=1` at rest.
- Direction: `+duty → +pos`; swap motor leads if reversed.
