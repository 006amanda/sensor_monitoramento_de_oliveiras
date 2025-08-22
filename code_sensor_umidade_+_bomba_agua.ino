const int PINO_UMIDADE = 34;  // Sensor AO -> GPIO34
const int PINO_RELE    = 22;  // Relé IN2 -> GPIO22

void setup() {
  Serial.begin(115200);
  pinMode(PINO_RELE, OUTPUT);
  digitalWrite(PINO_RELE, HIGH); // Relé desligado no início (ativo LOW)
}

void loop() {
  int leitura = analogRead(PINO_UMIDADE);

  // Calibração simples (ajuste conforme seus testes)
  int seco = 3500;   // valor típico solo seco
  int molhado = 1200; // valor típico solo molhado

  // Mapeia para 0–100% (inverso: quanto menor leitura, mais úmido)
  int umidade = map(leitura, seco, molhado, 0, 100);

  // Limita para 0–100%
  if (umidade < 0) umidade = 0;
  if (umidade > 100) umidade = 100;

  Serial.print("Leitura bruta: ");
  Serial.print(leitura);
  Serial.print(" | Umidade: ");
  Serial.print(umidade);
  Serial.println("%");

  // Controle da bomba
if (umidade < 20) {
  Serial.println("Solo seco! Ligando bomba...");
  digitalWrite(PINO_RELE, HIGH); // Liga bomba
  delay(1000);
  digitalWrite(PINO_RELE, LOW);  // Desliga
} else {
  digitalWrite(PINO_RELE, LOW);  // Mantém desligada
}


  delay(1000); // lê a cada 2s
}
