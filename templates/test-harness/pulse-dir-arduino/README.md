# test-harness: pulse-dir-arduino

Proven SAFE-IDLE bench-test firmware for **step/direction** motors on a 5 V Arduino
(generalized from the iSV57T bring-up). Counted bursts only — never free-running.

## Use
1. Copy `firstspin/` into your motor's `actuators/<slug>/test/`.
2. Edit the **CONFIG block** at the top of `firstspin.ino` to match `SPECS.md`:
   pins, `ACTIVE_LOW`, `halfPeriodUs` (rate), `burstPulses`, `DIR_SETUP_US`, `ALM_OK_LEVEL`.
3. Compile-check:
   ```bash
   arduino-cli compile --fqbn arduino:avr:uno actuators/<slug>/test/firstspin
   ```
4. Wire per `WIRING.md` (common-anode / active-LOW: +5V→PUL+/DIR+, Dx→PUL-/DIR-,
   GND→common, ALM via pull-up to A0).

## Serial commands (115200)
| key | action |
|---|---|
| `i` | status / read ALM |
| `f` / `r` | counted forward / reverse burst (**motion** — secure first) |
| `+` / `-` | change pulse rate |
| `]` / `[` | change burst count |

Boots IDLE; nothing moves until you press `f`/`r`. A `blink/blink.ino` smoke sketch for
Phase 1 lives alongside in the iSV57T example.

## Serial capture note
`arduino-cli monitor` can miss lines; opening the port auto-resets an Uno. A small
pyserial reader (open, `sleep(2)`, read) is more reliable.
