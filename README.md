# actuator-lab

**Point-of-truth library for the actuators we bench-test at Southern IoT.** One repo that
answers: *which motors/servos do we have, how do we safely drive each, and what's been
verified on the bench.*

Each actuator carries a uniform doc set (verified specs, wiring, safety gate,
commissioning log, results, handoff) plus its native bench-test firmware. Onboarding a
new motor is a **test process**, not just paperwork — every actuator earns its status by
passing the phase-gated [commissioning workflow](docs/COMMISSIONING-WORKFLOW.md).

## Catalog

| Actuator | Type | Control | MCU / Tool | Status | Folder |
|---|---|---|---|---|---|
| Leisai/Leadshine **iSV57T** (iSV5709V36T-01-1000) | Integrated BLDC servo | Pulse / DIR | Arduino Uno | ✅ validated (P0–P6) | [actuators/isv57t-servo](actuators/isv57t-servo) |
| **LEESN IG28ET** | Integrated stepper | Modbus RTU / RS-485 | ESP32-S3 (RAK3112) | ✅ bench-proven (jog + goto, 0-count err) | [actuators/leesn-ig28et-stepper](actuators/leesn-ig28et-stepper) |
| **LEESN IG42ET** | Integrated stepper | Modbus RTU / RS-485 | ESP32-S3 (RAK3112) | ✅ validated (P0–P7; 2.0 A, 4000 pp/rev, closed-loop, fault-recovery) | [actuators/leesn-ig42et-stepper](actuators/leesn-ig42et-stepper) |
| **REV Core Hex Motor** (REV-41-1300) | Brushed DC gearmotor (72:1) + encoder | PWM H-bridge (MC33886) | STM32 Discovery | ⚠️ retired at P5 — **gearbox mechanically damaged** (grinding; current-spike FS trips). P0–4 ✅ (288 cnt/rev cal). Superseded by MG513P30 | [actuators/rev-core-hex-motor](actuators/rev-core-hex-motor) |
| **MG513P30** 12V (Hall encoder) | Brushed DC gearmotor (30:1) + Hall quad encoder | PWM H-bridge (MC33886) | STM32 Discovery | ✅ validated (P0–7; 1456 cnt/rev/gearbox ~28:1; band ~20–290 RPM; cascade pos loop ±1 cnt repeat; FS debounce + stall detect) | [actuators/mg513p30-hall-motor](actuators/mg513p30-hall-motor) |
| **MG513 / GMR** (500 PPR, 30:1) | Brushed DC gearmotor + GMR quad encoder | DBH-12V H-bridge (20 kHz) | STM32 NUCLEO-F401RE | ⏳ **P0–6 ✅** (comms, encoder, open-loop, closed-loop RPM ss_err<0.5rpm band 0–200, position ±0°, repeat 0° drift/5cyc). P7 partial (E-STOP✅; stall/OC interactive pending). 4 local FW fixes (dir, UART-ORE wedge, baked gains, trap-vel). Firmware = submodule `stm32-nucleo-gmr-encoder` | [actuators/mg513-gmr-nucleo-f401](actuators/mg513-gmr-nucleo-f401) |
| Quanser **QUBE-Servo 2** | DC servo plant (model) | — (Simulink) | MATLAB | 📘 reference | [reference/qube-servo2-plant](reference/qube-servo2-plant) |

Status legend: ✅ validated (all gates) · ⏳ partial (Phases 0–N) · ⚠️ blocked at a gate ·
⛔ not started · 📘 reference/modeling. See [docs/CONVENTIONS.md](docs/CONVENTIONS.md).

## ⚠️ Safety first

Any command that **energizes or moves** a motor is gated. Read the actuator's
`SAFETY-CHECKLIST.md` and only command motion with the motor **secured, shaft
unloaded/clear, supply current-limited**. Read-only steps (param reads, status, bus
scans) are always safe. Full rules in [CLAUDE.md](CLAUDE.md).

## Layout

```
actuator-lab/
├── docs/        # COMMISSIONING-WORKFLOW (the test pipeline), ADD-A-MOTOR, CONVENTIONS
├── templates/   # _actuator-template/ (copy per motor) + test-harness/ (proven starting firmware)
├── actuators/   # one folder per driven motor (native folder or git submodule)
└── reference/   # modeling/design references (not driven actuators)
```

## Add a new actuator

Follow [docs/ADD-A-MOTOR.md](docs/ADD-A-MOTOR.md): copy the template + the matching test
harness, fill specs/safety from the manual, run the commissioning workflow Phases 0–6,
record each gate, then flip the catalog row above.

## Clone

```bash
git clone --recurse-submodules https://github.com/rnd-southerniot/actuator-lab.git
```

## License

Internal — Southern IoT (rnd-southerniot). Not for redistribution without permission.
