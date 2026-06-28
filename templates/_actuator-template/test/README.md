# test/

Bench firmware for this actuator. Copy the matching starting point from
[../../../templates/test-harness/](../../../templates/test-harness) by control class:

- `pulse-dir-arduino/` → step/direction motors on Arduino
- `modbus-rs485-espidf/` → Modbus RTU drives on ESP32

Then edit its **config block** to match `../SPECS.md`, compile-check, and use it to run
the commissioning workflow Phases 1–6.
