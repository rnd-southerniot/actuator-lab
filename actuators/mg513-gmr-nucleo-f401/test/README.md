# test/ — firmware lives in the submodule

This actuator's bench firmware is **not** in this folder — it's the vendored source repo at
[`../src/`](../src/) (`rnd-southerniot/stm32-nucleo-gmr-encoder`), which carries the full STM32Cube
firmware, host server, dashboard, and its own HW test harness.

- **Firmware:** [`../src/firmware/`](../src/firmware/) — `make` + OpenOCD → `build/dc-motor-pid.elf`.
- **Host / bring-up tools:** [`../src/host/`](../src/host/) — `server.py`, `calibrate.py`, `commission.py`,
  `hw_test/` (6-phase runner), `sysid.py`.
- **Do not edit firmware here.** Fix upstream in the submodule's repo and bump the pointer
  (`git submodule update --remote …` then commit).

See [../COMMISSIONING-LOG.md](../COMMISSIONING-LOG.md) for how the source repo's phases map to the
actuator-lab gates.
