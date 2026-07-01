# SPECS — MG513P30 12V (Hall encoder)  (canonical verified interface)

> `✅` = confirmed from the vendor spec sheet; `⏳` = bench-verify at commissioning.
> **Sources:**
> - [S3] **Vendor spec images** (from AliExpress item 4000996252848, provided 2026-07-01):
>   "Motor parameters" table (MG513P30_12V row) + "Encoder parameters and comparison" table
>   (Hall encoder row). **Canonical.**
> - [S1] Vendor listing (AliExpress item 4000996252848). [S2] Web cross-refs (indicative only).

## Motor — MG513P30_12V ✅ [S3]
| Parameter | Value |
|---|---|
| Rated voltage | **12 V DC** |
| Rated (running) current | **0.36 A** |
| Locked-rotor (stall) current | **3.2 A** |
| No-load output speed (after gearbox) | **366 ± 26 RPM** |
| Rated output speed | **293 ± 21 RPM** |
| Rated torque | **1 kg·cm** (≈ 0.098 N·m) |
| Stall (locking) torque | **4.5 kg·cm** (≈ 0.44 N·m) |
| Rated power | ~4 W |
| Gear ratio | **30:1** ("P30") |
| Frame / shaft | ~ϕ37 mm body, **6 mm D-shaft** output ⏳ confirm dims on the unit |

## Encoder — Hall (magnetic) ✅ [S3]
| Parameter | Value |
|---|---|
| Lines (PPR, motor shaft, per channel) | **13 PPR** |
| Type | Magnetic induction (Hall), 2-phase A/B quadrature |
| Supply range (VCC) | **3.3 – 5 V** |
| Output | **Push-pull, internally pulled up to VCC → read directly by MCU** (no external pull-up) |
| Back cover | Bare (no cover; stable, doesn't need one) |

### Derived unit constant (⏳ bench-verify at Phase 4)
- **Counts / output rev = 13 PPR × 4 (quadrature edge decode) × 30 (gear) = 1560 counts/rev** (nominal).
  - ×4 assumes the firmware's full-quadrature EXTI decode (both edges, both channels) — as on the REV rig.
  - **Do NOT trust 1560 in production** until confirmed against a shaft mark by the multi-rev
    accumulation method (same as REV's 288 verification).
- **Free-speed count rate = 1560 × 366/60 ≈ 9,516 counts/s** → sets `VEL_MAX_CPS` (use ~10,500 headroom).

## Drive / control interface (reuses the REV rig) ✅ rig / ⏳ bench
- Driver: **Waveshare 2× MC33886 H-bridge**, sign-magnitude, **PWM 1 kHz** (MC33886 enable limit — NOT 20 kHz).
- MCU: **STM32F429I-DISC1**. Motor + → MC33886 M1; PWM/DIR + FS(PB7) exactly as the REV build.
- Logic: STM32 3.3 V → board 74LVC8T245 (VCCA = 3.3 V). Feed board 3V3/VCCA solidly (Phase-3 lesson).

## Feedback / current
- **Encoder A/B → PC4/PC5 (EXTI edge decode)**; **VCC = 3.3 V** (in range; keeps A/B at 3.3 V, native-safe
  on the STM32 inputs — no divider). GND = common.
- Current sense: **INA238** (onboard 15 mΩ) inline in M1, I2C3 @0x40 — reuse rig. Running ~0.36 A is well
  within range; readings unreliable under PWM (use a meter for DC ground-truth), as established.

## Safety-relevant numbers (for the current-limit + fuse)
- **Stall 3.2 A @ 12 V.** Commissioning current-limit: set the PSU so it can deliver running current with
  margin but caps near/below stall — **~3.5 A limit, fused ~3–4 A**. Running draw is only ~0.36 A, so
  low-speed bring-up is gentle. (This is a much lighter electrical load than the REV unit.)

## Firmware deltas vs the REV Core Hex build (to apply in `test/`)
1. Velocity clamp `VEL_MAX_CPS` **700 → ~10500** (free speed ≈ 9516 cps). **Critical.**
2. Unit constant for reporting/model: **288 → 1560** counts/output-rev; gear **72 → 30**.
3. Encoder VCC wiring at **3.3 V** (REV was also 3.3 V — same).
4. Re-tune velocity/position gains (different plant); prior REV gains do not carry.

## Verified-on-bench
- Date/rig: pending Phase 0. See [RESULTS.md](RESULTS.md) and [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md).
