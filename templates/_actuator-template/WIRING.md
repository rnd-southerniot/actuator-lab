# WIRING — <MODEL>

Wire only with power OFF + discharge wait. Confirm every value against [SPECS.md](SPECS.md).

## POWER
```
   DC PSU <V>            <MODEL> POWER CONNECTOR
   (limit ~1A, fused)
     (+) ──[ FUSE ]────► +Vdc / V+
     (−) ─────────────► GND / V-
   First test: lowest valid V, current limit ~1A. Meter-verify polarity at the connector.
```

## CONTROL — <control class>
```
   <controller>                       <MODEL> CONTROL CONNECTOR
   <pin> ──(<R or direct>)──────────► <signal+>
   <pin> ──(<R or direct>)──────────► <signal->
   GND   ──────────────────────────► <common>     ◄── single common ground
   <ALM/fault read-back, input only — never drive>
```
> For pulse/dir: see `templates/test-harness/pulse-dir-arduino` for the common-anode /
> active-LOW reference. For Modbus: A/B + GND, DE/RE on the transceiver.

## DO / DON'T
```
  DO    power controller→idle FIRST, then motor supply; reverse on power-down
  DO    set DIR/direction before commanding motion; start with a small counted move
  DO    keep ONE common ground
  DON'T drive the ALM/fault line (it is an OUTPUT)
  DON'T hot-plug the control connector with power on
  DON'T <motor-specific gotcha from SPECS.md>
```

## Config switches (if any)
- <DIP/jumper settings for first test, from SPECS.md>. Set with power OFF.
