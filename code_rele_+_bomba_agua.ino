const int RELAY = 22;   // GPIO do ESP32 ligado no IN2 do módulo

void setup() {
  pinMode(RELAY, OUTPUT);
}

void loop() {
  digitalWrite(RELAY, LOW);   // muitos módulos ligam com nível LOW
  delay(1000);                // mantém 1 segundo ligado
  digitalWrite(RELAY, HIGH);  // e desligam com HIGH
  delay(1000);                // mantém 1 segundo desligado
}
