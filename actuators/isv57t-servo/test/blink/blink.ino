/* blink.ino - Uno smoke test. Onboard LED (D13) 1 Hz + serial heartbeat. */
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println(F("blink: Uno alive"));
}
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println(F("LED ON"));
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println(F("LED OFF"));
  delay(500);
}
