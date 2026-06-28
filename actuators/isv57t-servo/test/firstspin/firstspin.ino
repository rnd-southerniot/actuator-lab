/*
 * firstspin.ino - iSV5709V36T-01-1000 (iSV57T) bench first-motion test
 * Arduino (5V), common-anode / ACTIVE-LOW wiring:
 *   +5V -> PUL+ , DIR+      D9 -[R]-> PUL-      D8 -[R]-> DIR-      GND -> input common
 *   ALM+ -[pull-up 4.7k to +5V]- A0   ALM- -> GND   (read only, never driven)
 *
 * SAFETY: boots IDLE (no motion). Motion ONLY on explicit serial command.
 * Do NOT run a burst until: ACHSeries trial run passed, wiring per WIRING-DIAGRAM.md,
 * series-R confirmed (PRE-WIRING-CHECKLIST A), servo powered at 24V no-load, shaft clear.
 */

const int PIN_PUL = 9;     // -> R -> PUL-
const int PIN_DIR = 8;     // -> R -> DIR-
const int PIN_ALM = A0;    // <- ALM+ via pull-up (read only)

// ACTIVE-LOW: pin HIGH = opto OFF (idle).  pin LOW = active.
const uint8_t IDLE = HIGH;
const uint8_t ACTIVE = LOW;

// First-test motion parameters (deliberately tiny/slow).
long     burstPulses = 200;      // counted burst, NOT free-running
uint32_t halfPeriodUs = 500;     // 500us half => ~1 kHz (well under any max-freq)
const uint32_t DIR_SETUP_US = 20; // >= 5us spec, with margin

void idleOutputs() {
  digitalWrite(PIN_PUL, IDLE);
  digitalWrite(PIN_DIR, IDLE);
}

void setup() {
  pinMode(PIN_PUL, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ALM, INPUT);        // external pull-up provides level
  idleOutputs();                  // <-- safe state BEFORE anything else
  Serial.begin(115200);
  delay(200);
  banner();
}

void banner() {
  Serial.println(F("\n=== iSV57T firstspin :: SAFE IDLE ==="));
  Serial.print  (F("PUL=D9 DIR=D8 (active-LOW)  burst="));
  Serial.print(burstPulses); Serial.print(F(" pulses @ ~"));
  Serial.print(1000000UL / (2UL * halfPeriodUs)); Serial.println(F(" Hz"));
  Serial.println(F("Commands:"));
  Serial.println(F("  i = status / read ALM"));
  Serial.println(F("  f = FORWARD burst (counted)"));
  Serial.println(F("  r = REVERSE burst (counted)"));
  Serial.println(F("  + / - = change pulse rate   ] / [ = change burst count"));
  Serial.println(F("Outputs are IDLE. Nothing moves until you press f or r."));
}

void readAlm() {
  int a = digitalRead(PIN_ALM);
  Serial.print(F("ALM line = "));
  Serial.print(a == HIGH ? F("HIGH") : F("LOW"));
  Serial.println(F("  (confirm active-state meaning in PRE-WIRING-CHECKLIST D)"));
}

void doBurst(bool forward) {
  Serial.print(F(">> "));
  Serial.print(forward ? F("FORWARD ") : F("REVERSE "));
  Serial.print(burstPulses); Serial.println(F(" pulses..."));

  digitalWrite(PIN_DIR, forward ? ACTIVE : IDLE);  // set direction first
  delayMicroseconds(DIR_SETUP_US);                 // DIR ahead of PUL

  for (long i = 0; i < burstPulses; i++) {
    digitalWrite(PIN_PUL, ACTIVE);
    delayMicroseconds(halfPeriodUs);
    digitalWrite(PIN_PUL, IDLE);
    delayMicroseconds(halfPeriodUs);
  }
  idleOutputs();                                   // return to safe idle
  Serial.println(F(">> done. Outputs IDLE."));
  readAlm();
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
