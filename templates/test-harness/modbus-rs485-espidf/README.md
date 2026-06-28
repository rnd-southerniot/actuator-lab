# test-harness: modbus-rs485-espidf

Starting point for **Modbus RTU / RS-485** integrated drives on ESP32 (ESP-IDF). Rather
than duplicate code here, this harness points at the **bench-proven reference
implementation** and the gated console pattern to copy.

## Reference implementation
[`actuators/leesn-ig28et-stepper`](../../../actuators/leesn-ig28et-stepper) (submodule)
is a production-grade example. Reuse its components:

- `firmware/components/modbus/` — transport-free Modbus RTU codec (CRC16, FC03/04/06/16),
  host unit-tested.
- `firmware/components/rs485/` — half-duplex UART transport (DE/RE as inverted RTS).
- `firmware/main/app_main.c` — gated console REPL.

## Console pattern (map onto the commissioning workflow)
| command | phase | safe? |
|---|---|---|
| `scan` / `read` / `dump` | P2 comms sanity | ✅ read-only |
| `mot-read` (status/pos/speed/alarm) | P2 | ✅ read-only |
| `mot-enable` / `mot-disable` | P3 | ⚠️ motion |
| `jog-fwd` / `jog-rev` (bounded, auto-stop) | P3/P5 | ⚠️ motion |
| `goto <pos>` (distance-capped) | P4/P6 | ⚠️ motion |

## To onboard a new Modbus drive
1. Copy the leesn `firmware/` tree into `actuators/<slug>/firmware/` (or add as submodule
   if it's its own repo).
2. Replace the **register map** (`docs/<MODEL>_REGISTER_MAP.md`) and the motion register
   addresses with the new drive's manual values — keep the codec/transport.
3. Re-verify **32-bit word order** and unit IDs/baud for the new drive (these bite).
4. Run Phases 0–6, logging `COMMISSIONING-LOG.md`.

> Watch-outs from the LEESN bring-up: word order is LOW-word-first; encoder-line-count
> register is NOT an enable; motion writes are physical — gate them.
