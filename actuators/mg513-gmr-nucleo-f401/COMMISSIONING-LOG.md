# COMMISSIONING LOG — MG513/GMR + DBH-12V + NUCLEO-F401RE

Per-phase record for [../../docs/COMMISSIONING-WORKFLOW.md](../../docs/COMMISSIONING-WORKFLOW.md).
Catalog status = highest phase fully passed. Firmware + host live in the submodule [`src/`](src/)
(`rnd-southerniot/stm32-nucleo-gmr-encoder`) — its own 6-phase HW bring-up maps to these gates below.

Date: 2026-07-06 (onboarding)  Operator: Arif  Rig/MCU: NUCLEO-F401RE + DBH-12V + MG513 GMR (30:1)
Firmware: `src/firmware` → `dc-motor-pid.elf` (submodule pinned)

> **Cross-ref:** the source repo's [HARDWARE_STATUS.md](src/HARDWARE_STATUS.md) is authoritative for its
> own phase runs. This log tracks the actuator-lab gates; where a source phase already covers a gate it's
> cited, but motion gates (3–7) still need a fresh **written go-ahead** on *this* bench.

## Phase 0 — Bench safety setup  ⏳ pending
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Clamp / shaft clear / path clear | safe | | ☐ |
| Common ground (MCU+DBH+enc+batt = 1 node) | verified | | ☐ |
| Encoder on **5 V** (CN6-5), 12 V connected LAST | correct | | ☐ |
| SB13/14 ON, SB62/63 OPEN | correct | | ☐ |
| Supply current-limited (~2–3 A) + fused | set | | ☐ |

## Phase 1 — Controller smoke test  ✅ 2026-07-07 (bench-confirmed on this rig)
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Firmware builds + flashes | ok | `make` (BSS+DATA 5.6 KB) → OpenOCD program+verify OK, git `41e245c` | ✅ |
| Serial/console alive | heartbeat | `hw_test` Phase 1 **5/5**: ping 0.52 s; **111.2 Hz** (556 pkts); 0 gaps; boot IDLE; 0 dropped | ✅ |
- Reproduces the source repo's Phase-1 (5/5, 2026-04-05). Ran via
  `gmr-venv/bin/python -m hw_test.hw_runner --port <VCP> --phases 1` from `src/host` (COBS/CRC16 @921600).
- Also = **Phase 0** essentials: board on USB, boots DISARMED/IDLE, **12 V not connected**. ✅

## Phase 2 — Power-on & feedback sanity  *(read-only)*  ✅ 2026-07-07 (encoder on bench; **src Phase 2 unblocked**)
Coordinated hand-rotation capture (no 12 V) over the COBS link (`pos`=output-shaft deg, `rpm`=motor):
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Counts register on rotation | pos/rpm change | pos 190→258→22; rpm swung +609 … −1039 | ✅ |
| **CW = positive** | +pos / +rpm | CW → pos rose, rpm +609 | ✅ |
| CCW reverses | −pos / −rpm | CCW → pos fell to 22, rpm −1039 | ✅ |
| RPM → 0 stationary | rpm=0 held | ended flat pos=22, **rpm=0** | ✅ |
- Both encoder channels count, **direction = CW-positive**, zeroes at rest → A/B wiring + **5 V** good.
- Battery-monitor ADC (PB1) not yet checked (needs the divider + pack) — defer to Phase 3 with 12 V.
- Ran via `gmr_mon.py` (SerialFixture capture); `pos` is `pos_actual` float (deg), not raw counts —
  **raw CPR / 60 000-cnt-per-rev bench-verify at Phase 4.**

## Phase 3 — First motion  ✅ 2026-07-07 open-loop (go-ahead given) — ⚠️ closed-loop needs tuning
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Motor spins, encoder responds | motion + counts | DIAG open-loop (15 % duty): rpm +866, pos advanced | ✅ |
| Direction +duty → +RPM | forward = + | after sign fix (below): +duty → +rpm/+pos (was −) | ✅ |
| Reverse | −duty → −RPM | DIAG reverse step ran; motor+driver bidirectional | ✅ |
| CT / current with EN=HIGH | sane | pulled current while driven (vs idle floating ~3 A) | ⏳ quantify @P3.2 |

### ⚠️⚠️ RUNAWAY on first closed-loop attempt → firmware sign fix (READ)
- **First move used RPM mode (+15) → runaway to −10 434 RPM, output saturated 1.0** (positive feedback:
  +output spun the shaft the encoder reads as −, so the PID drove to max). Exactly WIRING.md note #5 /
  the sibling MG513P30's inverted encoder. **ESTOP'd; motor undamaged.**
- **Fix (user chose firmware over swapping MOTOR A/B):** negated the output in
  `src/firmware/Core/Src/motor.c` `Motor_SetOutput()` (`duty = -duty;`) → +output now → +RPM/+encoder,
  keeping the Phase-2 CW=+ convention. **⚠️ LOCAL edit to the `src/` submodule — NOT committed upstream**
  (to persist: commit in `stm32-nucleo-gmr-encoder` + bump the pointer; flagged with user).
- **Verified open-loop via DIAG** (bypasses PID): +duty → +866 rpm, +pos → direction correct; DIAG did
  NOT hit its "direction inverted" fail path. Rebuild gotcha: needed `make clean` — a plain `make` linked
  a stale `motor.o` (identical ELF size masked it).
- **⚠️ Closed-loop still unusable — default gains far too hot.** With correct sign, RPM mode now
  *oscillates* (bounded ±300, out bang-bangs ±1) instead of running away → `kp=0.5/ki=0.1` ≈ 1000× too
  high for this plant (15 % open-loop already ≈ 866 rpm). **Phase 4 = autotune / set sane gains (~1e-3).**

## Phase 4 — Closed-loop RPM  ✅ 2026-07-07 (go-ahead given) — tuned + baked
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Closed-loop tracks setpoint | rpm → sp | ss_err **+0.1 @80**, **−0.2 @150** rpm | ✅ |
| No output saturation | \|out\|<1 | 0% samples at \|out\|≥0.99; out_ss≈0.038 | ✅ |
| Setpoint ceiling honored | clamp @200 | sp=300 → 200 (MOTOR_MAX_RPM=200, this unit) | ✅ |
| Tuned gains persist | boot = tuned | boots kp=0.0015 ki=0.01 kd=0.0 after reset | ✅ |
- **Tuned gains: `kp=0.0015, ki=0.01, kd=0.0`** (RPM PID). Hand-tuned on-bench (relay autotune bails —
  see below). Stock `0.5/0.1/0.01` is ~300× too hot: 0.15 duty ≈ 866 rpm, so a 30-rpm error alone
  saturates the output → **bang-bang limit cycle** (rpm ±340, out ±1.0). At kp=0.0015 the loop is stable
  and non-saturating; ki=0.01 nulls the proportional droop to <0.5 rpm.
- **Noise:** velocity-estimate noise ≈ ±30 rpm (1σ) at steady state, but `out` is steady → shaft speed is
  steady; this is measurement noise (same trait as sibling MG513P30 before its windowed estimator).
- **Persistence via baked boot defaults** (controller.c `PID_Init`), NOT runtime flash-save — see FW-BUG-02.
  Matches the lab pattern ("bake tuned boot defaults", MG513P30).
- Closed-loop RPM band this config: **0–200 rpm** (telemetry unit; MOTOR_MAX_RPM setpoint clamp).

### 🔧 Firmware findings this session (all fixes LOCAL to the `src/` submodule — not committed upstream)
- **FW-FIX-01 — direction (Phase 3):** `motor.c` `Motor_SetOutput()` `duty = -duty;` (recorded above).
- **FW-BUG-01 / FW-FIX-02 — command-RX wedge (found + fixed):** at 921600 a command byte arriving while
  the 1 kHz control ISR runs overruns the single-byte `HAL_UART_Receive_IT` (ORE). The firmware had **no
  `HAL_UART_ErrorCallback`**, so on ORE the HAL aborts RX and never re-arms → **command RX dies permanently
  while telemetry TX keeps streaming** (looks like "board ignores commands"; only an ST-LINK reset cleared
  it). **Fix:** added `HAL_UART_ErrorCallback` in `main.c` that clears ORE and re-arms single-byte RX.
  Verified: 120 back-to-back commands (previously reliable wedge) now self-heal. This is why last session's
  closed-loop commands "wouldn't land" — a firmware bug, not the host scripts.
- **FW-BUG-02 — runtime flash-save resets the board (found; NOT fixed, sidestepped):** `CMD_SAVE_GAINS`
  → sector-7 erase blocks the CPU ~1–2 s inside `Flash_SaveGains`, but **IWDG (~400 ms) keeps counting**;
  the "kick watchdog during suspend" loop can't run during the blocking erase → **IWDG resets mid-erase**,
  leaving flash invalid so boot falls back to defaults. Confirmed: save → gains revert to 0.5/0.1/0.01;
  post-reset boot = defaults. **Worked around by baking tuned gains as boot defaults** (deterministic,
  reproducible). A proper fix would refresh IWDG immediately before erase + widen its reload, or move gains
  to a RAM-func erase — deferred (out of commissioning scope; flagged for the source repo).

## Phase 5 — Speed survey  ⚠️ go-ahead (src Phase 4: closed-loop RPM PID + autotune)
- Closed-loop RPM sweep; relay-feedback auto-tune available in firmware. Log tracking + noise.
- **NOTE:** firmware relay-autotune (`CMD_AUTOTUNE`) bails to FAILED on this plant (oscillation exceeds
  `AUTOTUNE_AMPLITUDE_MAX*MOTOR_MAX_RPM` safety before 3 clean cycles) → leaves gains unchanged. Hand-tune
  is the validated path here; autotune tuning is a future refinement (lower relay amp / raise safety band).

## Phase 5 — Position mode  ✅ 2026-07-07 (go-ahead given) — trap profile fixed
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Absolute move accuracy | err <2° | 90°→**0.0°**, 0°→**0.0°**, 360°(270° travel)→**0.0°** | ✅ |
| Trapezoidal motion | smooth ramp | clean accel/cruise/decel; no overshoot | ✅ |
| Deadband at target | out <5% | out→0.0 at target (holds, no buzz) | ✅ |
- **Cascade = TrapProfile (feedforward velocity) → RPM PID → motor** (pos_pid retained but unused).
- **FW-FIX-04 (config.h, LOCAL):** stock `TRAP_MAX_VEL_DEG_S=720`, `ACCEL=3600` demand ~3600 rpm at the
  motor (×GEAR_RATIO) but the loop clamps to MOTOR_MAX_RPM=200 (~40°/s output) — so the **time-based
  profile finished ~18× before the clamped motor arrived → 12° of a 90° move, then stopped**. Matched to
  the real limit: **`TRAP_MAX_VEL_DEG_S=30`, `ACCEL/DECEL=90`**. After fix all moves land at ~0° error.
- ⚠️ **hw_test `phase5` 5.1 (360°) FAILs on a harness timeout only** — its 8 s cap < 9 s needed at the
  correct 30°/s (270° travel). Verified separately with a 14 s capture: 360°→**360.0°, err +0.0**. 5.2/5.3/5.4
  PASS in-harness. Motion/accuracy are correct; the harness timeout was tuned for the unvalidated 720°/s.
- Rebuild gotcha: `config.h` is a header → **`make clean && make`** (plain `make` relinked stale objects,
  identical run twice until cleaned).

## Phase 6 — Repeatability  ✅ 2026-07-07 (go-ahead given)
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| Return-to-zero drift, 5× out-and-back (0↔180°) | <2° | mean **−0.02°**, **sd 0.00**, max \|drift\| **0.0°** | ✅ |
| Endpoint repeat @180° | tight | mean 180.01°, **sd 0.00** | ✅ |
- 1800° total travel, **zero lost counts** and tight deadband convergence over 5 cycles. Measures the
  closed-loop returning to its own encoder position (no dial indicator → not an *absolute* mechanical
  check, but 0-count drift is the key controller result). Ran via `scratchpad/pos_repeat.py`.

## Phase 7 — Fault handling & recovery  ⏳ 2026-07-07 partial (E-STOP + recovery ✅; stall/OC interactive pending)
| Check | Expected | Measured | Pass? |
|---|---|---|---|
| E-STOP under motion | mode→estop, out→0 | spun 185 rpm → **estop, out 0.000, rpm→0** | ✅ |
| Clear-fault → IDLE | recovers | CLEAR_FAULT → mode idle | ✅ |
| Fault flags clean at rest | 0 | fault=0 | ✅ |
| **Stall detection** | fault on hold | *needs interactive shaft-hold* | ⏳ |
| Overcurrent (CT, EN-gated) | trip on load | *needs mechanical load; overlaps stall* | ⏳ |
| IWDG watchdog recover | reset→recover | not induced (would need a deliberate hang) | ⏳ |
| Low-battery | BATT warn/crit | PB1 divider likely unpopulated — deferred | ⏳ |
- **E-STOP + fault recovery proven** (the core safety path). Ran via `scratchpad/estop_test.py` /
  `safety.py`. Lesson: send `SET_MODE=RPM` with a mode-readback retry when coming out of position-mode
  deadband — the first fire-and-forget can be a no-op (comms are reliable, but state transitions need a beat).
- **Stall/overcurrent** are the harness's *interactive* test (spin 60 rpm, hold the shaft): needs a fresh
  motion go-ahead + operator hands + care (30:1 output torque). Left for a focused Phase-7 session.

**Result:** ☐ ⏳ Phases 0–6 ✅; Phase 7 E-STOP/recovery ✅, stall/OC/IWDG/low-batt pending — catalog ⏳ P0–6.

---

# Axis B (Motor B) — dual-axis commissioning  ✅ 2026-07-07

Second MG513/GMR motor on the **same NUCLEO-F401RE** (auro-balancer dual-motor rig). Firmware:
dual-axis `stm32-nucleo-gmr-encoder` @ `5a76778` (branch `feat/dual-axis-motor-b`). Axis B runs an
independent TIM9 1 kHz control loop mirroring Axis A. **Axis A untouched (regression 5/5).**

## Wiring (verified against auro-balancer HARDWARE_TRUTH §3)
| Signal | Pin | Peripheral | AF |
|---|---|---|---|
| Motor B PWM CH1/CH2 | PB6 / PB7 | TIM4_CH1/CH2 | AF2 (20 kHz, ARR=4199) |
| Motor B EN | PC11 | GPIO out (active-HIGH, start LOW) | — |
| Encoder B CH1/CH2 | PA6 / PA7 | TIM3_CH1/CH2 | AF2 (16-bit) |
| Control ISR B | — | TIM9 (shared TIM1_BRK_TIM9 vector) | — |
| Motor B CT | PC0/IN10 in fw (map says PC4) | — | **disabled** — wiring unverified |

## Phase results
| Phase | Check | Measured | Pass? |
|---|---|---|---|
| 1 comms | 0xAB telemetry + CMD_*_B | ~100 Hz, per-axis seq contiguous, boots IDLE | ✅ |
| 2 encoder | hand-rotate → counts | 308° travel, bidirectional (rpm ±1400) | ✅ |
| 3 first motion + direction | +duty → +rpm | **inverted like Axis A → `dir_sign=-1`**; +25→+20 rpm, no runaway | ✅ |
| 4 closed-loop RPM | ss_err, saturation | 80 rpm: **ss_err +0.0**, out 0.034, 0% sat (baked gains as-is) | ✅ |
| 5 position | move accuracy | 90°/270° both ways → **±0.1°** err, no saturation | ✅ |
| 6 repeatability | 5× out-and-back drift | return drift **0.0°**, sd 0.00 | ✅ |
| 7 E-STOP under motion | mode→estop, out→0 | spun 128 rpm → estop, out 0.000 | ✅ |
| 7 clear-fault → IDLE | recovers | CLEAR_FAULT_B → idle | ✅ |
| 7 stall detection | fault on hold | held @60 rpm → **fault @2.5 s, flag 0x02**, clear→idle | ✅ |
| 7 overcurrent | — | **N/A** — Motor B CT disabled (PC0/PC4 wiring unverified) | ⏳ |

### ⚠️⚠️ Motor B direction inverted (like Axis A) — READ
First closed-loop move (+25 rpm) ran away to **−10050 rpm, out saturated 1.0** (positive feedback,
same as Axis A / WIRING #5). ESTOP'd; motor undamaged. Fixed by `g_motor_b.dir_sign = -1` in
`motor.c` (bench-verified; the parameterized firmware equivalent of swapping the B leads). Re-verified
correct. `dir_sign` is per-axis in the `MotorHandle` — Axis A stays −1, Axis B now −1.

### Notes
- Global fixes inherited by B: UART-ORE `HAL_UART_ErrorCallback` (command wedge), trap-vel=30
  (position). Per-axis: direction sign (bench), baked RPM gains 0.0015/0.01/0 (same plant as A).
- **Command delivery:** `SET_MODE_B`/`SET_POSITION_B` can no-op on the first fire-and-forget send;
  drive them with a telemetry read-back retry (`pos_sp`/`mode`) — then reliable. Same class as Axis A.
- **Open items:** Motor-B overcurrent (needs CT wiring confirmed — map says PC4/ADC1_IN14, note the
  HW-BUG-05 CT swap) then enable `ct_enabled`; and the map/firmware Motor-A EN divergence (PC10 vs PA10).

**Result:** ✅ Motor B validated Phases 2–7 (overcurrent N/A). Dual-axis rig complete: both motors
independent closed-loop RPM + position on one NUCLEO.
