# SPECS — MG513 (GMR encoder) + DBH-12V + NUCLEO-F401RE  (canonical verified interface)

> `✅` = confirmed from the source repo / bench; `⏳` = bench-verify during commissioning.
> **Source:** submodule [`src/`](src/) = `rnd-southerniot/stm32-nucleo-gmr-encoder` (do not edit from here).
> Cite it: [src/README.md](src/README.md), [src/WIRING.md](src/WIRING.md), [src/HARDWARE_STATUS.md](src/HARDWARE_STATUS.md).

## Identity
- Motor: **MG513** brushed DC gearmotor, **GMR quadrature encoder 500 PPR**, **30:1** gearbox. ✅ [src]
- MCU: **STM32 NUCLEO-F401RE** (84 MHz Cortex-M4F). Driver: **DBH-12V** dual H-bridge (3.3 V logic, **20 kHz PWM**).
- **Distinct from [mg513p30-hall-motor](../mg513p30-hall-motor/)** — that is the *Hall* 13-PPR variant on an
  F429+MC33886 rig; this is the *GMR* 500-PPR variant on an F401+DBH-12V rig. Don't cross the pin maps.

## Encoder / unit constant
- **GMR 500 PPR** (per motor-shaft rev) → ×4 quadrature (TIM2 encoder mode) = 2000 counts/motor-rev
  → × 30 gear = **60 000 counts / output rev** (nominal). ⏳ **bench-verify @ Phase 4** (the sibling
  MG513P30 gearbox measured 28:1 not the nominal 30 — don't assume).
- Interface: **TIM2 (32-bit) hardware quadrature**, A/B on PA0/PA1.
- Encoder supply: **5 V** (NUCLEO CN6 pin 5) — **NOT 3.3 V** (GMR won't run reliably at 3.3 V). ✅ CRITICAL [src/WIRING]

## Power
- **12 V 3S 18650** pack → NUCLEO **VIN (CN7 pin 24)** *and* DBH-12V **VM**; common ground. 7–12 V VIN range. ✅
- Motor: MG513 rating ⏳ confirm (running/stall current, no-load RPM) — capture at Phase 4/5. Sibling MG513
  variants: ~0.3 A run, ~2–3 A stall; free speed a few-hundred RPM output.
- **Battery monitor:** 100 kΩ/33 kΩ divider Battery+ → **PB1 (ADC1_IN9, CN10-24)** + 100 nF; 12.6 V → 3.13 V. ✅

## Control interface (DBH-12V)
- **PWM 20 kHz**, dual-input sign: **IN1=PA8 (TIM1_CH1)**, **IN2=PA9 (TIM1_CH2)**, **EN=PA10 (GPIO)**.
- **Current sense CT → PA4 (ADC1_IN4)**, analog 0–3.3 V. ⚠️ **never exceed 3.3 V on PA4**; CT **floats when
  12 V absent** → firmware gates the overcurrent check on EN=HIGH (HW-BUG-01). ✅ [src]
- Logic levels: DBH-12V inputs are 3.3 V-compatible — **no level shifting needed**.

## Comms / protocol
- **USART2 → ST-LINK VCP** (PA2 TX / PA3 RX), **921600 baud**, **COBS framing + CRC-16**. ✅
- **SB13/SB14 ON** (route USART2 to VCP); **SB62/SB63 must remain OPEN** (USART2 bus-conflict risk). ✅
- Host stack (in submodule): Python FastAPI server + React dashboard; `host/` tools (calibrate, commission,
  sysid, hw_test). 1 kHz MCU control loop (TIM6 ISR); modes IDLE/RPM/POSITION/AUTOTUNE/DIAG/FAULT/E-STOP.

## Firmware (in submodule `src/firmware/`)
- STM32Cube HAL + Makefile, `arm-none-eabi`, OpenOCD flash → `build/dc-motor-pid.elf`. BSS+DATA < 78 000 B.
- **Do not build/edit firmware from actuator-lab** — it lives in the submodule; fix upstream + bump the pointer.

## Pin map (NUCLEO-F401RE) ✅ [src/README, src/WIRING]
| Function | MCU pin | Periph | Header | To |
|---|---|---|---|---|
| Encoder A | PA0 | TIM2_CH1 | CN7-28 (A0) | ENC CH-A |
| Encoder B | PA1 | TIM2_CH2 | CN7-30 (A1) | ENC CH-B |
| PWM fwd | PA8 | TIM1_CH1 | CN10-23 (D7) | DBH IN1 |
| PWM rev | PA9 | TIM1_CH2 | CN10-21 (D8) | DBH IN2 |
| Motor EN | PA10 | GPIO out | CN10-33 (D2) | DBH EN |
| Current sense | PA4 | ADC1_IN4 | CN7-32 (A2) | DBH CT |
| Battery mon | PB1 | ADC1_IN9 | CN10-24 | 100k/33k divider |
| UART TX/RX | PA2/PA3 | USART2 | ST-LINK VCP | Host USB |

**⚠️ SUPERSEDED — dual-axis update (verified 2026-07-07; source: config.h + COMMISSIONING-LOG.md "CT/EN reconciliation, closed").** The single-axis pin rows above are stale. Bench-verified current truth for the dual-axis rig:

| Signal | Axis A | Axis B |
|---|---|---|
| Motor PWM | PA8/PA9 (TIM1) | PB6/PB7 (TIM4) |
| Encoder | PA0/PA1 (TIM2) | PA6/PA7 (TIM3) |
| Motor EN | **PC10** (was PA10) | **PC11** |
| Current sense (CT) | **PC3 / ADC1_IN13** (was PA4/IN4) | **PC4 / ADC1_IN14** |
| Control ISR | TIM5 (was TIM6) | TIM9 |

Both axes: `ct_enabled=true`, overcurrent latch **live** (EN-gated, >8 A → brake); bench-confirmed reading real current (idle ~0, driven 0.05–0.20 A, no false trip). The **>8 A trip itself has not been empirically forced** — see verification follow-up.

## Verified-on-bench
- **Both axes fully commissioned 2026-07-07** (Axis A Phases 1–6; Axis B Phases 2–7 incl. overcurrent + stall + E-STOP). Dual-axis rig complete. (Legacy line below predates commissioning.)
- Source repo status (legacy): **Phase 1 (serial comms) 5/5 PASS** (2026-04-05); Phase 2 (encoder) pending wiring.
  Actuator-lab commissioning re-runs the gates in this repo's structure — see [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md).
