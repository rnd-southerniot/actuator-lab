# SAFETY CHECKLIST — REV Core Hex Motor (MC33886 / STM32)

Two parts: **A–F verified facts** (fill from datasheets) and **G per-session gate** (re-check every
session). Don't wire control until A–F are filled.

Refs: REV Core Hex Motor docs (2026-06-29); **Waveshare RPi Motor Driver Board** schematic (2× MC33886,
74LVC8T245, LM2596); STM32F429I-DISC1 + `rnd-southerniot/st-discovery` `docs/stm32f429i-disc1-pin-mapping.md`.

- STM32 3.3 V → board header **PWMA (Pin 37) + M1 (Pin 38) + M2 (Pin 40)** through the **74LVC8T245**
  translator; **supply 3V3 to Pi-header Pin 1** (translator A-side / VCCA ref) + shared GND (Pin 34/39). PWM
  ~20 kHz; M1/M2 = direction, PWMA = active-high enable. **Power-select switch OFF** (board must not
  back-feed 5 V to the F429). ☐

## B. Comms / tuning port level
- STM32 USB (ST-LINK VCP / SWD) for flashing + telemetry. No high-voltage pin to a PC. ☐

## C. Signal timing (PWM)
- PWM frequency vs MC33886 max switching / propagation; dead-time if locked-antiphase. ☐

## D. Fault output (board FS1)
- FS1 = open-drain fault, **pulled up to 5 V on the board (R7 1k)** → read on a **5 V-tolerant** F429
  pin or via a divider (never a non-FT/analog pin). **Active logic** ‹confirm: LOW = fault›; firmware
  trips PWM→0 on fault. ☐

## E. Feedback / resolution
- Quadrature **288 counts/rev (output)** / 4 CPR motor; encoder 3.3 V. Unit constant + velocity
  method (input-capture T/M) recorded in MODELING.md. ☐

## F. Power supply
- **12 V**; motor **stall 4.4 A** → MC33886 (~5 A) is near limit at stall. First test: **12 V,
  current limit ~1.5 A, fused.** Raise limit deliberately. Watch driver heat at stall. ☐

## G. Per-session mechanical/safety gate  *(every session)*
- ☐ Motor body clamped; **5 mm hex output secured** (geared 3.2 N·m can whip a shaft/load), path & fingers clear
- ☐ PSU current-limited + fused; polarity metered at the JST-VH
- ☐ MC33886 EN/disable in safe state at power-up; STM32 boots PWM = 0
- ☐ Power-up order: STM32 (USB) idle FIRST, then 12 V motor supply
- ☐ Discharge-wait acknowledged before any rewiring; 12 V isolated from STM32/encoder (3.3 V)

### GATE
☐ A–F verified from datasheets (no guesses; resolve the TBDs: board variant, MC33886 pinout, encoder
pinout, FS polarity) · ☐ G re-checked this session · ☐ **written go-ahead before any motion phase
(3–7).** If any blank → stop.
