# WIRING — MG513/GMR + DBH-12V + NUCLEO-F401RE

Wire only with power OFF. **Canonical source:** [src/WIRING.md](src/WIRING.md) (submodule — the wiring
diagrams, colours, and second-channel map live there). This page is the actuator-lab summary + safety gate.

## Power (connect 12 V LAST)
| Source | Destination | Note |
|---|---|---|
| 3S 18650 (+) | NUCLEO **VIN CN7-24** | 7–12 V regulator input |
| 3S 18650 (+) | DBH-12V **VM** | motor power |
| 3S 18650 (−) | NUCLEO **GND CN7-22** + DBH **GND** | **common ground — same node** |

## DBH-12V ↔ MCU
| DBH pin | MCU | Header | Function |
|---|---|---|---|
| IN1 | PA8 | CN10-23 (D7) | PWM forward (TIM1_CH1, 20 kHz) |
| IN2 | PA9 | CN10-21 (D8) | PWM reverse (TIM1_CH2) |
| EN | PA10 | CN10-33 (D2) | H-bridge enable (GPIO) |
| CT | PA4 | CN7-32 (A2) | current sense (ADC1_IN4, **0–3.3 V max**) |
| GND | GND | CN7-22 | logic ground |
| MOTOR A / B | M+ / M− | — | motor terminals (swap to flip direction) |

## Encoder (MG513 GMR) ↔ MCU
| Enc | MCU | Header | Function |
|---|---|---|---|
| **VCC** | **+5 V** | **CN6-5** | **5 V — NOT 3.3 V** (GMR needs 5 V) |
| GND | GND | CN7-22 | common |
| CH-A | PA0 | CN7-28 (A0) | TIM2_CH1 |
| CH-B | PA1 | CN7-30 (A1) | TIM2_CH2 |

## Battery voltage monitor (add before first 12 V run)
`Battery+ ── 100 kΩ ── PB1 (CN10-24, ADC1_IN9) ── 33 kΩ ── GND`, + 100 nF PB1→GND.
1 % resistors; 12.6 V → 3.13 V at PB1 (safe for 3.3 V ADC). Meter the divider before connecting.

## Solder-bridge config (NUCLEO-F401RE) — verify
- **SB13 / SB14 ON** → USART2 routed to the ST-LINK VCP (host comms). ✅ (Phase-1 verified in src)
- **SB62 / SB63 OPEN** → avoids a USART2 bus conflict. ⚠️ must stay open.

## DO / DON'T
- **DO** power the encoder from **5 V** (CN6-5). At 3.3 V it won't read reliably.
- **DO** connect the **12 V battery LAST**, after logic wiring is verified and the MCU boots fault-free.
- **DON'T** exceed **3.3 V on PA4** (CT). Add a divider/clamp if the DBH CT can swing higher on fault.
- **DON'T** trust the CT reading with 12 V absent — it floats (HW-BUG-01); the firmware gates the
  overcurrent check on EN=HIGH.
- **Direction:** if RPM reads negative when it should be positive, swap **MOTOR A/B** (or fix the sign in
  firmware upstream).

## Power-up / down order
1. USB (ST-LINK) → MCU boots DISARMED. 2. Verify comms/encoder (Phases 1–2, no 12 V). 3. **Then** 12 V.
Down: 12 V OFF first, then USB.
