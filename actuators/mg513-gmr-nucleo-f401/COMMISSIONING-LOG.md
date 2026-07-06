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

## Phase 4 — Scaling / calibration  ⚠️ go-ahead
- **Unit constant: 60 000 counts = 1.000 output rev** (500 PPR × 4 × 30) → **bench-verify** against a
  shaft mark (multi-rev method; sibling gearbox was 28:1). Measured: ______

## Phase 5 — Speed survey  ⚠️ go-ahead (src Phase 4: closed-loop RPM PID + autotune)
- Closed-loop RPM sweep; relay-feedback auto-tune available in firmware. Log tracking + noise.

## Phase 6 — Repeatability  ⚠️ go-ahead (src Phase 5: position mode)
- Cascaded position→velocity; e.g. 360° = one output rev, N out-and-back cycles, drift.

## Phase 7 — Fault handling & recovery  ⚠️ go-ahead (src Phase 6: safety systems)
- Overcurrent (CT, EN-gated), stall detection, E-STOP/brake, IWDG watchdog, low-battery. Verify
  detect → fail-safe → recover.

**Result:** ☐ ✅ validated (Phase 7 passed) — update catalog row.
