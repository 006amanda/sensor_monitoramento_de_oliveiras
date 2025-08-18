#define PINO_SENSOR 34   // Pino analógico para o sensor de umidade (AO)
#define PINO_RELE   23   // Pino digital para o módulo relé

// Valores de calibração (ajuste conforme testes no seu sensor)
int valorSeco = 3500;     // Valor quando o solo está seco
int valorMolhado = 1200;  // Valor quando o solo está úmido

void setup() {
  Serial.begin(115200);

  pinMode(PINO_RELE, OUTPUT);
  digitalWrite(PINO_RELE, HIGH); // Relé desligado (muitos módulos ativam em LOW)

  Serial.println("=== Sistema de Irrigação - Teste Iniciado ===");
}

void loop() {
  // Leitura do sensor de umidade
  int leitura = analogRead(PINO_SENSOR);

  // Converte para percentual (%)
  int umidade = map(leitura, valorSeco, valorMolhado, 0, 100);
  umidade = constrain(umidade, 0, 100);

  Serial.print("Leitura bruta: ");
  Serial.print(leitura);
  Serial.print(" | Umidade: ");
  Serial.print(umidade);
  Serial.println("%");

  delay(1000); // Faz leitura a cada 1s
  }
}
