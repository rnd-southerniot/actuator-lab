# MODELING — MG513P30 12V (Hall)  ·  bench system-ID

Preliminary plant identification for a Simulink/control model. Data captured 2026-07-03 on the
validated rig (F429 + MC33886 + MG513P30, unloaded, 12 V) using the firmware's **1 kHz `cap` burst
logger** (on-MCU timing — far cleaner than the 10 Hz STATUS stream).

## What this is / isn't
- **Is:** an input→output (PWM **duty** → output-shaft **velocity**) model — a first-order linear
  approximation around the operating region, plus the measured steady-state curve and the dominant
  nonlinearities.
- **Isn't:** a separated physical DC-motor model (Ra, La, Ke, Kt, J, b individually). That needs
  **current** measurement, and the onboard INA238 is unreliable under PWM (established in commissioning).
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

## Limitations / next steps
- **No current data** → Ke, Kt, Ra, La, J, b not separated. Add a PWM-synchronous or averaged current
  measurement (or a series shunt read at PWM-off), then a locked-rotor test (Ra), no-load spin-down
  (J, b), and back-EMF (Ke) give the physical parameters.
- Unloaded only — inertia/friction under a real load will differ.
- Duty→voltage assumed ~linear at 12 V; the MC33886 drop + PWM make it approximate.
