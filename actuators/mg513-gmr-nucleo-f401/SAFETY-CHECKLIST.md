# SAFETY CHECKLIST — MG513/GMR + DBH-12V + NUCLEO-F401RE

Two parts: **A–F verified facts** and **G per-session gate** (re-check every bench session).
Refs: [src/WIRING.md](src/WIRING.md), [src/HARDWARE_STATUS.md](src/HARDWARE_STATUS.md), [SPECS.md](SPECS.md).

## A. Control-input drive requirement
- DBH-12V logic inputs **3.3 V-compatible — no level shifting**. IN1=PA8, IN2=PA9 (20 kHz PWM), EN=PA10.
  Motor is disabled unless EN=HIGH. ✅

## B. Comms / tuning port level
- **USART2 → ST-LINK VCP** (PA2/PA3), USB, 921600 baud, COBS+CRC-16. **SB13/14 ON, SB62/63 OPEN.**
  No pin here is hazardous to a PC (standard VCP). ✅

## C. Signal timing (PWM drive)
- 20 kHz complementary PWM on TIM1 (IN1/IN2); not a step/dir drive. ✅

## D. Fault / current-sense output
- **CT (DBH) → PA4 (ADC1_IN4)**, analog **0–3.3 V max** — **never exceed 3.3 V on PA4**. ⚠️
- CT **floats when 12 V is absent** → do NOT use it as a fault trigger without 12 V; firmware gates the
  overcurrent check on **EN=HIGH** (HW-BUG-01, fixed upstream). Firmware also has stall detection + IWDG. ✅

## E. Feedback / resolution
- **GMR 500 PPR → 60 000 counts/output-rev** nominal (×4 × 30). ⏳ **bench-verify @ Phase 4** (sibling
  gearbox was 28:1, not 30). Encoder VCC = **5 V** (CN6-5), **not 3.3 V**. ✅ CRITICAL

## F. Power supply
- **12 V 3S 18650** → VIN (CN7-24) + DBH VM; 7–12 V VIN range; common ground. First energize:
  current-limit ~**2–3 A**, fused; **connect 12 V LAST**. ⏳ confirm MG513 stall current before setting.
- 18650 packs **sag under load** (sibling-motor lesson) — a stiff bench PSU is preferred for any
  characterization/current work.

## G. Per-session mechanical/safety gate  *(every session)*
- ☐ Motor clamped, shaft unloaded, rotation path & fingers clear
- ☐ Encoder on **5 V**; **12 V connected LAST**; polarity metered at the connector
- ☐ PSU/pack current-limited + fused; common ground verified (one node)
- ☐ Solder bridges correct with power OFF (SB13/14 ON, SB62/63 OPEN)
- ☐ Power-up: USB/logic FIRST (boot fault-free), then 12 V; down in reverse

### GATE
☐ A–F verified from src/manual (no guesses) · ☐ G re-checked this session · ☐ **written go-ahead before
any motion phase**. If any blank → stop; resolve from the source repo / vendor.
