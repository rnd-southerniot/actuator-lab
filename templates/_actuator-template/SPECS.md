# SPECS — <MODEL>  (canonical verified interface)

> Every value below MUST be cited to the manual. Fill `<…>` and check ✅ once verified.
> **Source:** <manual name>, <version>, <date> — <link/page refs>.

## Identity
- Part number: `<MODEL>` — frame <NEMA?/mm>, power <W>, voltage <V nominal>, encoder <lines → counts/rev>.

## Power
- Supply range: <min–max V> (recommended <…>), continuous current <A>, peak <A>.
- Regen/back-EMF note: <…>

## Control interface
- Method: <pulse/dir | Modbus RTU | CAN | analog | …>
- Logic levels: HIGH <V>, LOW <V>; input current <mA>; isolated? <yes/no>.
- Series resistor / buffer needed? <none (internal limit) | value | 5V buffer for 3.3V>.
- Timing (if pulse): min pulse width <µs>, DIR setup <µs>, max freq <kHz>.
- Direction / mode config: <…>

## Feedback / alarm
- Encoder: <lines / counts-per-rev / interface>.
- ALM/fault output: type <OC/opto/…>, rating <mA @ V>, **active logic** <normal=?, alarm=?>.

## Comms / tuning port (if any)
- Type/levels: <RS-232 / TTL / USB>; isolated? <…>; baud <…>; cable/software <…>.
- ⚠️ Gotchas: <e.g. a +5V pin that must not connect to a PC>.

## Config switches / parameters
- <DIP table or key register/parameter list with verified values>.

## Verified-on-bench
- Date/rig: <…>. See [RESULTS.md](RESULTS.md) and [COMMISSIONING-LOG.md](COMMISSIONING-LOG.md).
