# MODELING — MG513P30 12V (Hall)  ·  bench system-ID

Preliminary plant identification for a Simulink/control model. Data captured 2026-07-03 on the
validated rig (F429 + MC33886 + MG513P30, unloaded, 12 V) using the firmware's **1 kHz `cap` burst
logger** (on-MCU timing — far cleaner than the 10 Hz STATUS stream).

## Physical DC-motor parameters (2026-07-04) — motor shaft
After fixing the INA238 averaging (validated: 0.24 A INA = 0.24 A PSU) the electromechanical parameters
were identified. **Reflect to the output shaft with the ~28:1 gear** (ω_out = ω_mot/28; T_out = T_mot·28).
| Param | Value | Source / confidence |
|---|---|---|
| **Ra** (armature) | **5.1 Ω** | multimeter across motor leads, shaft rotated (lowest-average). Solid. PWM locked-rotor couldn't resolve it (discontinuous conduction → 1.7–9 Ω scatter). |
| **Ke = Kt** | **0.0133 V·s/rad (N·m/A)** | free-spin: slope of (V_app − I·Ra) vs ω_mot. ~±15%. |
| **b** (viscous) | **1.96×10⁻⁶ N·m·s/rad** | steady-state Kt·I = b·ω + T_c. Rough. |
| **T_coulomb** | **1.27×10⁻³ N·m** | same. Rough. |
| **J** (rotor) | **~1–3×10⁻⁶ kg·m²** | spin-down (b·τ, τ=515 ms) = 1.0e-6; from accel τ_m = 2.8e-6. |
| **La** | not measured | small (~tens of µH); a fast electrical pole, negligible for control. |
- Sanity: Ra ⇒ stall 12/5.1 ≈ **2.4 A @ 12 V** (datasheet 3.2 A optimistic); Ke ⇒ no-load ≈ **307 RPM**
  (measured ~320–350). Consistent to ~10–15 %.

**Equations (motor shaft):**
```
   V = I·Ra + Ke·ω            (La ignored)
   J·dω/dt = Kt·I − b·ω − T_c·sign(ω)
   ⇒  ω(s)/V(s) = Kt / (J·Ra·s + (b·Ra + Kt·Ke))   [linear, no Coulomb]
      DC gain ≈ 71 rad/s/V (motor) ≈ 24 RPM/V (out);  τ_m = J·Ra/(b·Ra+Kt·Ke) ≈ 27 ms
```
The measured *accel* τ (~80 ms) is longer than the linear τ_m (~27 ms) because of the **Coulomb friction**
(the same nonlinearity as the accel/decel asymmetry) — include T_c for fidelity.

## Black-box view (duty→speed) — what this is / isn't
- **Is:** also an input→output (PWM **duty** → output-shaft **velocity**) first-order model around the
  operating region (below), useful when you don't want the physical block.
- **Isn't:** a high-fidelity friction model — Coulomb + breakaway are characterized but not finely fit.
  See "Limitations" — the electrical parameters are left as future work with better current sensing.

## Steady-state characteristic (open-loop, `steady_state_map.csv`)
| duty (0–999) | output speed | | duty | output speed |
|---|---|---|---|---|
| 600 | 5351 cps (221 RPM) | | 900 | 7283 cps (300 RPM) |
| 750 | 6524 cps (269 RPM) | | 999 | 8515 cps (351 RPM) |
- **Local slope ≈ 8 cps/duty** (≈ 0.36 RPM/duty) over 600–999. The curve is mildly **concave** (flattens
  toward the ~366 RPM no-load spec) and does **not** extrapolate through the origin — a Coulomb-friction
  offset. **Breakaway from rest ≈ 550–600 duty** (won't start below that).
- Duty→voltage (approx): duty 999 ≈ 12 V (nominal; less the MC33886 drop). So ≈ **30 RPM/V** at the
  output, ≈ 3.16 (rad/s)/V.

## Dynamic response (1 kHz step captures)
- **Accel** step 600→999 (`step_up_600_999_1kHz.csv`): clean **first-order**, **τ ≈ 80 ms**
  (LSQ log-fit 85 ms, 63%-crossing 72 ms — agree), local gain **K ≈ 8.8 cps/duty**.
- **Decel** step 999→600 (`step_down_999_600_1kHz.csv`): **≈ 190 ms and non-exponential** (slope decays
  15→8 cps/ms). The **accel/decel asymmetry (~80 vs ~190 ms) is the Coulomb-friction signature** — a
  single linear τ does not describe both directions.

## Model
**First-order linear (valid small-signal, in the linear operating band, accel):**
```
   ω_out(s) / duty(s)  ≈  K / (τ·s + 1),   K ≈ 8.8 cps/duty (≈ 0.36 RPM/duty),  τ ≈ 0.08 s
   in SI (output shaft):  ω_out(s)/V(s)   ≈  3.16 / (0.08·s + 1)   (rad/s per volt)
```
**With friction (recommended for fidelity)** — series/parallel add to the linear block:
- **Coulomb + breakaway:** a dead-zone / static-friction term ≈ **575 duty** equivalent before motion
  from rest; asymmetric drag that makes decel slower than accel.
- **Viscous** term is folded into τ and the steady-state slope.

## Building it in Simulink (guidance — MATLAB not run here)
1. Import the CSVs (`readtable`). Input = `duty`, output = `vfilt_cps` (÷1456×60 → RPM), t = `i_ms`/1000.
2. Quick check: `sys = tf(K,[tau 1])` with the values above; `lsim` against `step_up_*` — should overlay.
3. Better: `iddata`/`tfest` (System Identification Toolbox) on `step_up_600_999` for a refined
   1st/2nd-order fit; validate on the *other* step. Expect a good accel fit, larger decel residual.
4. Add a **Coulomb & Viscous Friction** block (Simscape) or a dead-zone to capture breakaway + asymmetry;
   tune against `step_down_*` and the `steady_state_map`.
5. For a physical DC-motor block, you still need Ke/Kt/Ra/J/b — see Limitations.

## Data files (`docs/sysid/`)
- `step_up_600_999_1kHz.csv` — 600 samples @1 kHz, accel step (τ source).
- `step_down_999_600_1kHz.csv` — 500 samples, decel step (friction).
- `steady_state_map.csv` — duty vs steady speed.
- Recapture anytime: `arm` → `jog <d1> 900` → `cap <d2> <n≤900>` (open-loop jog at d2, logs n samples
  @1 kHz, dumps `CAP_BEGIN … i_ms,vfilt_cps,duty … CAP_END`).

## Current sensing (2026-07-03)
- **Fixed the INA238 averaging:** the firmware never wrote `ADC_CONFIG` (0x01), so it ran at the
  power-on default `AVG=1` — a single conversion caught the PWM at a random phase → the wild 0–1200 mA
  swings seen in commissioning. Now set to **`ADC_CONFIG=0xF904` (AVG=128, ~145 ms integration)** →
  reports the **true average** current: e.g. steady **~135 mA at `vel 3000`** (was unusable). ~7 Hz update
  (fine for steady-state; too slow for transients — the `cap` logger uses encoder/duty, not current).
- **Still NOT accuracy-validated.** Needs an external reference (bench-PSU current display or a DC ammeter
  in series with a motor lead) — the INA sits on the PWM'd motor node (rail-to-rail common mode), so a
  residual scale/offset bias can't be ruled out from the reading alone.

## Limitations / residual uncertainty
- **Ra** is solid (multimeter). **Ke/Kt** ~±15 %; **b, T_c, J** are rough (small no-load currents; J spans
  1–3×10⁻⁶). Refine with a loaded sweep (varies I → tightens Ke/Ra) and a proper Coulomb+viscous fit.
- **La** not measured (small; fast pole, negligible for control-band modeling).
- Method notes that bit us (for the next motor): (1) **PWM locked-rotor can't give Ra** — discontinuous
  conduction; use a **multimeter** across the leads. (2) The INA **bus** (motor-node) voltage is unusable
  under PWM — only the **current** is trustworthy (after AVG=128). (3) Watch the **PSU current limit** —
  ours was <1 A and folded the rail, corrupting every high-current reading until raised.
- Unloaded only — inertia/friction under a real load will differ.
