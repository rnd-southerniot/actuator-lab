# COMMISSIONING LOG — REV Core Hex Motor

Per-phase record for [../../docs/COMMISSIONING-WORKFLOW.md](../../docs/COMMISSIONING-WORKFLOW.md)
(Phases 0–7). Fill measured/pass for each gate. Catalog status = highest phase fully passed.
**Motion phases (3–7) need the written safety go-ahead.**

Date: 2026-06-30  Operator: Arif  Rig/MCU: STM32F429I-DISC1  Driver: MC33886 (Waveshare RPi Motor Driver Board)  Firmware: Steps 1–4, branch `rev-core-hex-firmware` @ 884591b
Toolchain: Arm GNU 15.2.rel1 (vendored) · st-flash 1.8.0 · telemetry USART1 115200 via ST-LINK VCP (/dev/cu.usbmodem1103)

## Phase 0 — Bench safety setup
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Body clamped / hex output secured / path clear | safe | | ☐ |
| PSU 12 V, current-limited + fused | ~1.5 A, fused | | ☐ |
| Polarity at JST-VH | correct | | ☐ |
| 12 V isolated from STM32/encoder (3.3 V) | isolated | | ☐ |

## Phase 1 — Controller smoke test  ✅ 2026-06-30
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Firmware builds + flashes (STM32) | ok | `st-flash ... verified, jolly good`; chipid 0x419, 2 MB | ✅ |
| Console/telemetry alive (VCP/SWO) | heartbeat | STATUS @ 10 Hz on USART1; green LED ~5 Hz heartbeat | ✅ |
| PWM = 0 at boot (idle), EN safe | idle | boots `state=DISARMED→FAULT`, `mode=IDLE`, `duty=0` | ✅ |

## Phase 2 — Power-on & feedback sanity  *(read-only)*  ✅ 2026-06-30
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Idle current | ~0, steady | `cur_ma=0` at rest, DISARMED, 12 V bus up | ✅ |
| MC33886 FS (fault) | clear | `fs=1` once board 5 V up; `reset` cleared FAULT→DISARMED | ✅ |
| Encoder hand-rotate: A/B count up/down, direction sane | clean quadrature | CW: 0→294 (+vel); CCW: 294→7 (−vel); monotonic, no jumps | ✅ |
| Counts-per-output-rev hand-check | ≈ 288 | ~294 counts / 1 hand-rev (+2%, hand imprecision) → confirms 288 | ✅ |
| Current-sense reads 0 at rest, scales with load | sane | reads 0 at rest; bus `12330 mV` matches ~12.3 V PSU. Load-scale → Phase 3 (needs drive) | ✅* |

Params recorded: encoder CPR≈**288** (294 meas/hand-rev) · FS polarity=**active-LOW** (LOW=fault, HIGH=ok) · no-load current=**0 mA** (DISARMED) · INA238 bus scale **verified** (12.33 V); current-LSB scaling to be confirmed under drive at Phase 3.
Round-trip: 0→294→7 (residual 7 cnt ≈ 9°, hand imprecision — no systematic edge loss).
Console RX confirmed (VCP bidirectional): `reset` → `OK reset`.

**Phases 0–2 PASS. ⛔ STOP — Phase 3 (first commanded motion) requires written go-ahead.**

## Phase 3 — First motion (SAFE-IDLE)  ✅ 2026-07-01 (go-ahead given; motor body-clamped, shaft free)
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Smallest PWM step fwd | small defined move | `jog 250` (25% @ 1 kHz): pos 0→61 smooth, vel ~+110 cps, cur ~300–550 mA | ✅ |
| Reverse (DIR flip) | reverses, encoder sign flips | `jog -250`: pos 65→4, vel ~−105 cps, **signed current −630 mA**, fs clear | ✅ |
| FS / current | clear, within limit | fs=1 throughout; current sane; bus held ~8–10.5 V (no brownout) | ✅ |

### ⚠️ ROOT-CAUSE FINDINGS (cost ~a session of debugging — read before next bring-up)
1. **PWM frequency: PWMA drives the MC33886 *enable*, which CANNOT switch at 20 kHz.** At 20 kHz the bridge
   never enabled → both outputs floated to +12 V → zero drive. Waveshare's own demo PWMs it at **500 Hz**;
   firmware changed **20 kHz → 1 kHz** (TIM3 prescaler) and the motor drove instantly. (SPECS.md had flagged
   the 20 kHz as "confirm against MC33886 limits" — it was wrong.)
2. **VCCA (3V3→header Pin 1) must be a SOLID connection.** A flaky jumper left VCCA floating <1 V → the
   74LVC8T245 translator passed control signals only intermittently. Re-seated → steady 2.917 V → fixed.
   (Board 3V3/VCCA runs fine at ~2.9 V; the Discovery 3V3 sags under LCD+peripheral load but stays in spec.)
3. **INA238 current AND bus readings are UNRELIABLE during PWM** (shunt on a PWM'd lead, high CM dv/dt — the
   WIRING.md caveat). It reported bus "6.5 V" while a meter showed VIN steady at 12.5 V; current read low.
   **Use a multimeter for ground truth during drive.** Readings are trustworthy at rest / DC.
4. Direction convention: **+duty → +pos (forward), −duty → −pos**; INA238 returns signed current (− in reverse).
5. Tooling: telemetry + console both work over the **ST-LINK VCP** (/dev/cu.usbmodem1103) — RX confirmed.
   Note: ~25% duty draws ~0.5 A and the 18650+BMS rail dips to ~8–10 V (no bulk cap yet) — fine, but a
   ~1000–2200 µF cap across VIN would steady it and soften spin-up inrush (one inrush fault seen at 100%).

## Phase 4 — Scaling / calibration  ✅ 2026-07-01 (go-ahead given)
- Unit constant: **288 counts = 1.000 output rev** → **CONFIRMED.** Measured by **multi-rev
  accumulation** (more robust than discrete landings — see method note): a gentle `vel 100` spin moved
  **896 counts**; operator read the output-shaft mark as **3 full turns + ~40° past start** = 3.111 rev.
  → **896 / 3.111 = 288.0 counts/rev.** Predicted offset for 288 cnt/rev was ~40°, observed ~40°.
- **Uncertainty ≈ ±0.9%** (≈ ±3 counts), from the ~±10° eyeball angle read over 3.11 rev.
- **Method note:** the discrete 90°/180°/360° "land-on-mark" rows below were **superseded** — the
  position loop has a ~±13-count stiction/backlash deadband (see Phase 4 findings) that makes
  single-move landings unreliable, and aggressive position moves FS-trip the rail. The multi-rev
  velocity method decouples the calibration from stop precision and amplifies any per-rev error.
| Magnitude | Predicted | Measured | Pass? |
|---|---|---|---|
| multi-rev accumulation | 896 ct → 3 turns + ~40° (if 288 cnt/rev) | 3 turns + ~40° → **288.0 cnt/rev** | ✅ |
| ~~discrete 72/144/288 landings~~ | superseded by multi-rev (deadband makes landings ±13 ct) | n/a | — |

### ⚠️ Phase 4 finding — position loop has a stiction/backlash deadband (logged for the next bring-up)
Firmware `MODE_POS` is a **direct position→duty PID** (not cascaded). On-bench tuning (`gainp`):
- `kp=4, ki=0` → clean, no overshoot, but **stalls ~28 ct short** (proportional deadband; final duty
  112 < stiction breakaway ~180).
- `kp=4, ki=20` → **overshoots ~28 ct**, reverses, **limit-cycles**.
- `kp=2, ki=6` → gentle approach but still **overshoots ~26 ct**, then stiction *holds* it there until
  the integral winds to ~−180 duty and it crawls back → slow limit cycle.
- **Net:** effective positioning deadband ≈ **±13 counts (±16° at the output)**. Breakaway ≈ 180 duty
  (18%). **Recommended fix (next firmware rev):** cascade position→the (already-good) velocity loop, or
  add velocity + stiction feedforward. Out of scope for this commissioning pass.

## Phase 5 — Speed survey  ⏳ PARTIAL 2026-07-01 — **supply-limited** (go-ahead given)
- Closed-loop velocity. **Low speed validated; mid/high blocked by the bench supply (not the controller).**
- **Root cause of the block:** the bare **18650 + BMS supply (no bulk cap) sags under load and the
  MC33886 asserts an undervoltage FS trip → firmware faults safe.** The trip threshold *tightened as the
  cell discharged* over the session: `vel 288` (60 RPM) tripped after ~1 s at ~1.2 A / rail ≈ 10 V; later
  even `vel 100` (which held cleanly for 9 s early on) tripped in <1 s. **Action item: add a
  1000–2200 µF cap across VIN and/or use a stiff bench PSU**, then re-run mid/high. (Same fix flagged in
  Phase 3 findings.)
| Speed (RPM out) | Smooth? | Vel-est noise | Pass? |
|---|---|---|---|
| low (~10 → `vel 48`) | yes — vfilt holds 47 cps, range [46..50] | raw vel_cps ±3 cps (span 6) | ✅ |
| mid (~60 → `vel 288`) | loop **reached & held 288 cps** ~1 s, then **FS-tripped** (rail ≈10 V, 1.2 A) | n/a | ⚠️ supply-limited |
| high (~120, band limit) | not attempted — same supply limit (more current) | — | ⛔ blocked |

## Phase 6 — Repeatability  ⚠️ go-ahead
| Metric | Expected | Measured | Pass? |
|---|---|---|---|
| N out-and-back position cycles | returns home | | ☐ |
| Drift (account for backlash) | bounded | | ☐ |
| Fault events | 0 | | ☐ |

## Phase 7 — Fault handling & recovery  ⏳ PARTIAL 2026-07-01 — fail-safe + recovery demonstrated (incidentally)
- Not a deliberate Phase-7 run, but the **MC33886 undervoltage FS trip** induced by rail sag (Phase 5)
  exercised the whole chain **repeatably** this session, so it is recorded here.
- **Fault source seen:** FS1 (PB7) asserted LOW on supply undervoltage. NB: the firmware's *only* path to
  `ST_FAULT` is the FS pin (EXTI + 1 kHz backstop poll); there is **no software over-current trip**, so an
  INA238 current limit would NOT latch a fault on its own — worth adding for the stall case below.
- **Not yet exercised:** deliberate locked-output **stall** (mechanical) and MC33886 **over-temp**.
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Drive/firmware flags fault | FS asserts and/or current-limit trips PWM→0 | FS LOW → `ST_FAULT` via EXTI, latched | ✅ (FS path) |
| Fails safe | motor stops, no runaway | PWM→0 immediately; encoder count preserved; no runaway | ✅ |
| Reset/recover | clear cause → re-enable → fault clears | `reset` (FS clear) → DISARMED → `arm` → ARMED, repeatably | ✅ |
| Healthy motion resumes | small move, no fault | low-speed motion resumed post-reset | ✅ |
| Deliberate stall / over-temp | FS or (future) current-limit trips | not attempted this session | ☐ |

**Result:** ☐ ✅ validated (Phase 7 passed) — update catalog row.
**Session 2026-07-01 outcome:** Phase 4 ✅, Phase 5 ⏳ (supply-limited), Phase 7 ⏳ (FS path proven;
stall/over-temp pending). **Blocker for Phases 5–7 completion = stiff VIN supply (bulk cap / bench PSU).**

## ⛔ RETIRED 2026-07-01 (session 2) — gearbox mechanically failed
Re-attempting the Phase 5 speed survey, the drive FS-tripped repeatedly the moment current crossed
**~1 A**, and the threshold **worsened with cumulative run-time** (early `vel 100` held 9 s; later even
`vel 120` cut out in 6 s). It was **not** the supply: swapping to a bench PSU **and** raising its current
limit to 3 A did not move the threshold. Root cause was found to be **mechanical** — the operator heard
**grinding/crunching from the gearbox**. A binding/damaged gear train draws current spikes → crosses the
FS/undervolt point → trips; the progressive worsening = progressive gear damage. Likely **aggravated by
the hard direction reversals and stall-against-stiction holds during the direct-position-loop tuning**
(the ±13-count deadband limit-cycles) — a lesson for the next unit: cascade the position loop / avoid
repeated hard reversals on a delicate gear train.

**Disposition:** unit retired for precision work. **Superseded by
[MG513P30 (Hall)](../mg513p30-hall-motor/)** on the same rig (12 V DC + quad encoder). Phases 0–4 results
above remain valid as a record. The **firmware, FS-glitch hardening idea (RC filter / debounce on PB7),
and cascade-position-loop recommendation carry forward** to the MG513P30 build.
