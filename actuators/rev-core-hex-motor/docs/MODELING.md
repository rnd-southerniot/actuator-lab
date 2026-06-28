# MODELING — REV Core Hex Motor (MATLAB/Simulink)

Plan for the **mathematical model → system-ID → closed-loop control** workflow. Pairs with the bench
commissioning (Phases 0–7) and the STM32 firmware. Nothing here moves the motor — it's the model and
the experiment design.

## 1. Plant model (continuous)
Brushed DC motor + 72:1 gearbox + load. Electrical and mechanical coupling:

```
  Va = Ra·ia + La·(dia/dt) + Ke·ωm           (armature)
  Tm = Kt·ia                                 (torque)
  J·(dωm/dt) = Tm − b·ωm − Tload/N − Tfric   (mechanics, motor side)
  ωout = ωm / N,   θout = θm / N             (N = 72 gearbox; add backlash)
```

Parameters to identify: `Ra, La, Ke, Kt (≈Ke in SI), J, b`, Coulomb friction `Tfric`, gearbox
ratio `N=72`, and **backlash** (deadband on θ). For a brushed DC motor `Kt ≈ Ke` (SI units).

## 2. System-ID experiments (map to commissioning phases)
| Quantity | Experiment | Needs |
|---|---|---|
| `Ra` | locked-rotor, small DC step, V/I (Ohm) | current sense (ADC) |
| `La` | locked-rotor, current step rise time (electrical τ = La/Ra) | fast current ADC + scope/log |
| `Ke` | spin at known ω (open-loop PWM), measure back-EMF / steady V–ω | encoder velocity |
| `Kt` | steady current vs measured torque, or `Kt≈Ke` | current sense + (optional) torque |
| `J, b` | velocity step / coast-down (exp decay τ = J/b) | encoder velocity (T-method) |
| `Tfric` | min PWM to start motion (break-away) | encoder |
| backlash | reverse-direction position deadband | encoder, output mark |

> **Current sense unlocks Ra, La, Kt** — via the **INA238** (I²C): it returns **synchronized current
> + bus voltage + die-temp**, so you get V and I together (ideal for the electrical fit). ⚠️ I²C read
> cadence (conv-time + averaging) sets the current-sampling rate — fine for ID and a moderate torque
> loop; a fast inner current loop would want analog. For `La` (fast electrical τ), use INA238's
> shortest conversion + low averaging, or capture a step with a scope.

## 3. Velocity estimation (the low-resolution-encoder problem)
288 counts/rev (output) → **~600 counts/s** at 125 RPM → **<1 count per 1 ms sample**. Options:
- **T-method (period):** input-capture timestamps between edges → good at low speed, noisy at high.
- **M-method (count):** counts/interval → good at high speed, quantized at low.
- **M/T hybrid:** switch by speed. Recommended on STM32 (timer encoder + input capture).
- Filtering: track-with the model (a simple **Kalman/Luenberger observer** on θ,ω using the plant)
  beats raw differencing for this CPR — natural Simulink block.

## 4. Control architecture (cascade)
```
  θ* ─►[ Position P/PID ]─►ω* ─►[ Velocity PI ]─►i*/duty ─►[ (opt) Current PI ]─► PWM ─► plant
                                   ▲                          ▲
                                   └── ω̂ (observer) ──────────┴── ia (ADC)
```
- Inner **current/torque loop** (optional first cut): fast, gives torque control + stall limiting.
- **Velocity PI**, then outer **position** loop. Anti-windup on all integrators.
- Feedforward from the identified model (Ke·ω, friction) improves tracking.
- Loop rate: 1 kHz control ISR; current loop can run faster if used.

## 5. Simulink deployment
- Build the plant (§1) + observer (§3) + controller (§4) in Simulink; validate against ID data (§2).
- Deploy to STM32 via **Embedded Coder / STM32 support** (or Waijung), or hand-port the tuned gains
  into the C firmware. Use **External Mode** for live gain tuning + signal logging on the bench.
- Keep the identified parameters + tuned gains in [RESULTS.md](../RESULTS.md).

## 6. Open questions to close on the bench
- Actual `Kt/Ke`, `J`, `b` (vendor gives none) → must be measured.
- Backlash magnitude (limits position repeatability — Phase 6).
- Usable velocity band where T/M estimation is clean enough for a stable loop.
