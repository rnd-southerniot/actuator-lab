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
- **Step 3 ✅ first motion-capable build** (still boots DISARMED, PWM=0): INA238 current/voltage on I2C3
  (@0x40), DISARMED/ARMED/FAULT state machine, MC33886 FS-trip (PB7 EXTI falling **+ 1 kHz backstop poll**),
  gated serial console, and a 1 kHz TIM6 PID loop (velocity + position, anti-windup, **default gains 0**).
  NVIC: encoder + FS EXTI = prio 2 (FS is EXTI line 7, shares the EXTI9_5 vector with PC5 → checked first),
  PID = 6, console UART = 8, SysTick = 15. Runtime assert guards APB1×2 == 90 MHz.
- Step 4 — optional LCD live-plot.

## Console (115200 8N1) — motion is gated
| cmd | effect | gate |
|---|---|---|
| `status` | STATUS line (also auto @ 10 Hz) | — |
| `arm` / `disarm` | enter/leave ARMED | `arm` refused if FS faulted |
| `jog <duty> <ms>` | open-loop, bounded ≤2000 ms, auto-stop | ARMED |
| `vel <cps>` / `pos <counts>` | closed-loop setpoint | ARMED |
| `gainv <kp> <ki>` / `gainp <kp> <ki> <kd>` | set PID gains (default **0** → no motion until tuned) | — |
| `stop` | mode→IDLE, output 0 (stays armed) | — |
| `reset` | clear FAULT → DISARMED | only if FS clear |

Boots `DISARMED`. The motor cannot move until `arm` **and** a motion command **and** (for vel/pos) non-zero
gains. FS LOW → FAULT (EXTI + poll); `apply_output()` masks the FS EXTI around its gate-check+compare write
so a fault can't be overwritten by a stale duty (verified via arm-cortex-expert review).

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
