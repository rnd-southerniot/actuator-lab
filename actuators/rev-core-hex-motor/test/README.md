# test/

Bench firmware for the REV Core Hex Motor. Control class: **PWM H-bridge + quadrature DC servo** —
build from
[../../../templates/test-harness/pwm-hbridge-encoder-stm32/](../../../templates/test-harness/pwm-hbridge-encoder-stm32).

This is a **new control class** (neither `pulse-dir-arduino` nor `modbus-rs485-espidf` fits a
PWM-driven brushed DC servo with an encoder).

Target: **STM32F429I-DISC1** + **Waveshare RPi Motor Driver Board (2× MC33886)** + **INA238** (I²C
current/V). The firmware project lives **here** as an `st-discovery`-style Makefile build
(arm-none-eabi + `st-flash`), reusing that repo's proven F429 motor/encoder pin map.

Build steps (after the pin-conflict audit in `../WIRING.md` is closed):
1. Scaffold the CubeMX/Makefile project here; set pins per `../WIRING.md`, current via INA238 on I2C3.
2. Boot to **PWM = 0 idle**; gated console for jog/goto/velocity, FS-fault trip, INA238 current/V +
   encoder telemetry (optionally on the on-board LCD).
3. Compile-check (`make -j`), then run the commissioning workflow Phases 1–7, logging `../COMMISSIONING-LOG.md`.
