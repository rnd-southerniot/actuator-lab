# NEXT SESSION — MG513P30 12V (Hall)

Handoff note. Written **2026-07-01** (onboarding — motor not yet wired). Read first.

## Where we are
| # | Item | State |
|---|---|---|
| 1 | Specs confirmed from vendor sheet | ✅ [SPECS.md](../SPECS.md) — 13 PPR, 30:1, 366±26 RPM, 0.36 A run / 3.2 A stall, enc 3.3–5 V push-pull |
| 2 | Firmware ported from REV build + compiles | ✅ `test/` — `ENC_COUNTS_PER_OUT_REV=1560`, `VEL_MAX_CPS=11000`; `make` clean |
| 3 | Docs (SPECS, WIRING, README) | ✅ ; SAFETY-CHECKLIST + COMMISSIONING-LOG = template, fill at Phase 0 |
| 4 | Motor physically wired | ⛔ per [WIRING.md](../WIRING.md) |
| 5 | Phases 0–6 bench bring-up | ⛔ not started |

## Exact next steps
1. **Wire the motor** per [WIRING.md](../WIRING.md): motor leads → MC33886 M1; **encoder VCC → 3.3 V**
   (not 5 V); A/B → PC4/PC5; verify lead colours with a meter first.
2. **Bench PSU:** current-limit **~3.5 A** (stall is 3.2 A), fused. Running draw is only ~0.36 A.
3. **Flash:** `cd test && make flash` (ST-LINK). Boots **DISARMED / PWM=0**.
4. **Phase 0–2 (read-only / first energize):** hand-rotate shaft → `pos` counts, ~**1560 counts/rev**
   (verify exactly at Phase 4); `fs=1` at rest; INA238 bus ≈ supply at rest.
5. **Phase 3+ need written go-ahead.** Re-tune gains (set each session — firmware boots gains=0):
   start `gainv` low; the plant is different from REV (16× finer encoder, lower current, faster).

## Carried-over lessons (from the REV build — don't repeat)
- **Gains are NOT persisted** — re-apply `gainv`/`gainp` each session.
- **INA238 unreliable under PWM** — meter for DC ground-truth; good at rest.
- **Position loop:** the REV direct position→duty PID had a stiction deadband and limit-cycled, and the
  hard reversals likely **killed the REV gearbox**. For this (delicate) gear train: **cascade position →
  velocity** or add feedforward before Phase 6; avoid repeated hard reversals / stall-holds.
- **FS/PB7 is glitch-sensitive** (falling-edge EXTI, no filter). If false FS trips reappear under load,
  add an **RC filter on PB7** (1 k + 100 nF) and/or a firmware debounce (confirm LOW persists) — a real
  MC33886 fault is sustained, an EMI glitch is not.

## Serial / console
- USART1 **115200 8N1**, ST-LINK VCP (`/dev/cu.usbmodem*`). Verbs: `status arm disarm stop reset
  jog <duty> <ms> vel <cps> pos <counts> gainv <kp> <ki> gainp <kp> <ki> <kd>`.
- `cps = RPM/60 × 1560` (e.g. 60 RPM = 1560 cps; free speed ≈ 9500 cps).
- Helper used during REV bring-up: a small pyserial send/seq/monitor script (recreate in scratchpad).

## One-line resume
```bash
cd /Users/robotics/Developer/projects/robotics/actuator-lab/actuators/mg513p30-hall-motor && \
  $EDITOR WIRING.md && cd test && make flash   # after motor is wired + PSU limited ~3.5 A
```
