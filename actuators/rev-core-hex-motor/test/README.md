# test/ — REV Core Hex Motor bench firmware

Control class: **PWM H-bridge + quadrature DC servo**. Built as an `st-discovery`-style
Makefile project (arm-none-eabi + `st-flash`), reusing the proven F429 motor/encoder pin
map. Target: **STM32F429I-DISC1** + **Waveshare RPi Motor Driver Board (2× MC33886)** +
**INA238** (I²C current/V). Pin map + datasheet citations: [../WIRING.md](../WIRING.md),
[../SPECS.md](../SPECS.md), and `Core/Inc/main.h` (the firmware's single source of truth).

## Build status — staged (compile-gated)
- **Step 1 ✅ skeleton:** 180 MHz clock, motor driver forced idle, heartbeat LED. Boots PWM = 0.
- **Step 2 ✅ I/O bring-up:** TIM3_CH1 PWM (PB4, ~20 kHz, **started at 0% = coast**), DIR (PA5/PA7),
  FS input (PB7, read-only), EXTI quadrature encoder (PC4/PC5) + TIM2 1 µs edge-timestamp velocity,
  USART1 115200 `STATUS,pos=…,vel_cps=…,fs=…,duty=…` telemetry @ 10 Hz. **Still no motion** (duty never
  set non-zero). EXTI4/EXTI9_5 vectors wired in `startup.c`.
- Step 3 — INA238 driver (0x40), 1 kHz PID ISR, **FS-trip latch (EXTI on PB7)**, gated console (jog/goto).
  NVIC plan (from arm-cortex-expert review): **PB7 fault EXTI highest** (preempt 0–2) → cuts PWM first;
  **encoder EXTI (5) must out-prioritize the TIM6/7 PID (≥6)** so velocity is fresh when PID samples it.
  Also add a runtime assert that `HAL_RCC_GetPCLK1Freq()×2 == 90 MHz` so a clock-tree change can't silently
  drift the 20 kHz PWM / 1 µs timebase.
- Step 4 — optional LCD live-plot.

## Toolchain
- Needs a **complete** `arm-none-eabi-gcc` (WITH newlib), plus `make`.
  > ⚠️ Homebrew's `arm-none-eabi-gcc` formula is **compiler-only — no newlib** (`#include <stdint.h>`
  > fails: `include_next: No such file or directory`). Use the **Arm GNU Toolchain** instead. The cask
  > `gcc-arm-embedded` needs `sudo` (a `.pkg`), so on this machine it's **vendored without admin** by
  > extracting the cask's downloaded pkg payload:
  > ```sh
  > brew fetch --cask gcc-arm-embedded
  > pkgutil --expand-full "$(brew --cache --cask gcc-arm-embedded)" /tmp/armgnu
  > mv /tmp/armgnu/Payload "$HOME/Developer/projects/vendor/arm-gnu-toolchain-15.2.rel1"
  > ```
  > The Makefile auto-detects `~/Developer/projects/vendor/arm-gnu-toolchain-*/bin`. Verify a real
  > path with `arm-none-eabi-gcc -print-file-name=libc.a` (not a bare `libc.a`).
- `st-flash` (`brew install stlink`) — only needed to flash hardware (a gated step, not the build).

## HAL drivers (SHARED_DIR)
CMSIS + STM32F4 HAL are **not vendored here**; the Makefile pulls them from the local
`rnd-southerniot/st-discovery` clone (36 MB tree). Default:
`$(HOME)/Developer/projects/robotics/st-discovery/firmware/lcd-hello-f429i-disc1`. Override:
```sh
make SHARED_DIR=/path/to/st-discovery/firmware/lcd-hello-f429i-disc1
```
> TODO(portability): vendor a trimmed `Drivers/` (or add st-discovery as a submodule) so the build
> is self-contained and doesn't depend on an external untracked path.

## Build / flash
```sh
make -j4          # compile gate → build/rev-core-hex-f429i.elf + .bin
make flash        # st-flash write @ 0x08000000  (HARDWARE — gated; needs go-ahead + secured motor)
make clean
```

## Bring-up
Wire per [../WIRING.md](../WIRING.md), current-limit ~1.5 A, then run the commissioning workflow
Phases 0–7, logging [../COMMISSIONING-LOG.md](../COMMISSIONING-LOG.md). **Phases 3–7 (motion) need
written go-ahead** per [../SAFETY-CHECKLIST.md](../SAFETY-CHECKLIST.md).
