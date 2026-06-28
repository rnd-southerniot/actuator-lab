/*
 * firstspin.ino — generic SAFE-IDLE pulse/DIR bench-test harness (Arduino, 5V)
 * actuator-lab :: templates/test-harness/pulse-dir-arduino
 *
 * Drives a step/direction motor through the commissioning workflow Phases 1-6:
 * boots IDLE (no motion); moves only on an explicit serial command; counted bursts,
 * never free-running. Common-anode / ACTIVE-LOW wiring assumed (MCU pin LOW = active).
 *
 * >>> EDIT THE CONFIG BLOCK to match the motor's SPECS.md, then compile-check:
 *     arduino-cli compile --fqbn arduino:avr:uno <thisdir>
 * SAFETY: do not send f/r until SAFETY-CHECKLIST passes and motor is secured/no-load.
 */

// ======================= CONFIG (per motor) =======================
const int PIN_PUL = 9;     // -> PUL- (direct or via series R per SPECS.md)
const int PIN_DIR = 8;     // -> DIR-
const int PIN_ALM = A0;    // <- ALM via pull-up (read only); set -1 if unused

const bool    ACTIVE_LOW   = true;   // common-anode sinking: pin LOW = active
long          burstPulses  = 200;    // counted burst (NOT free-running)
uint32_t      halfPeriodUs = 500;    // 500us half => ~1 kHz; lower = faster
const uint32_t DIR_SETUP_US = 20;    // DIR-ahead-of-PUL (>= motor spec, e.g. 5us)
const uint32_t SERIAL_BAUD  = 115200;
// ALM read meaning is motor-specific; set how "OK" reads (see SPECS.md ALM logic):
const int     ALM_OK_LEVEL  = LOW;   // e.g. iSV57T: normal=LOW
// ==================================================================

const uint8_t IDLE   = ACTIVE_LOW ? HIGH : LOW;
const uint8_t ACTIVE = ACTIVE_LOW ? LOW  : HIGH;

void idleOutputs() { digitalWrite(PIN_PUL, IDLE); digitalWrite(PIN_DIR, IDLE); }

void setup() {
  pinMode(PIN_PUL, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  if (PIN_ALM >= 0) pinMode(PIN_ALM, INPUT);
  idleOutputs();                       // safe state BEFORE anything else
  Serial.begin(SERIAL_BAUD);
  delay(200);
  banner();
}

void banner() {
  Serial.println(F("\n=== firstspin :: SAFE IDLE ==="));
  Serial.print(F("PUL/DIR active-")); Serial.print(ACTIVE_LOW ? F("LOW") : F("HIGH"));
  Serial.print(F("  burst=")); Serial.print(burstPulses);
  Serial.print(F(" @ ~")); Serial.print(1000000UL/(2UL*halfPeriodUs)); Serial.println(F(" Hz"));
  Serial.println(F("i=status  f=FWD burst  r=REV burst  +/- rate  ]/[ burst count"));
  Serial.println(F("Outputs IDLE. Nothing moves until you press f or r."));
}

void readAlm() {
  if (PIN_ALM < 0) { Serial.println(F("ALM not wired")); return; }
  int a = digitalRead(PIN_ALM);
  Serial.print(F("ALM = ")); Serial.print(a == HIGH ? F("HIGH") : F("LOW"));
  Serial.println(a == ALM_OK_LEVEL ? F("  (OK)") : F("  (** ALARM **)"));
}

void doBurst(bool forward) {
  Serial.print(F(">> ")); Serial.print(forward ? F("FWD ") : F("REV "));
  Serial.print(burstPulses); Serial.println(F(" pulses..."));
  digitalWrite(PIN_DIR, forward ? ACTIVE : IDLE);
  delayMicroseconds(DIR_SETUP_US);
  for (long i = 0; i < burstPulses; i++) {
    digitalWrite(PIN_PUL, ACTIVE); delayMicroseconds(halfPeriodUs);
    digitalWrite(PIN_PUL, IDLE);   delayMicroseconds(halfPeriodUs);
  }
  idleOutputs();
  Serial.println(F(">> done. IDLE.")); readAlm();
}

void loop() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case 'i': readAlm(); break;
    case 'f': doBurst(true);  break;
    case 'r': doBurst(false); break;
    case '+': if (halfPeriodUs > 50) halfPeriodUs -= 50;
              Serial.print(F("rate ~")); Serial.print(1000000UL/(2UL*halfPeriodUs)); Serial.println(F(" Hz")); break;
    case '-': halfPeriodUs += 50;
              Serial.print(F("rate ~")); Serial.print(1000000UL/(2UL*halfPeriodUs)); Serial.println(F(" Hz")); break;
    case ']': burstPulses += 50; Serial.print(F("burst=")); Serial.println(burstPulses); break;
    case '[': if (burstPulses > 50) burstPulses -= 50; Serial.print(F("burst=")); Serial.println(burstPulses); break;
    case '\n': case '\r': break;
    default: banner(); break;
  }
}
