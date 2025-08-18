#define RE 4   // Receiver Enable
#define DE 5   // Driver Enable

HardwareSerial mod(2);  // Serial2 (UART2 no ESP32)

byte values[21];  // Array para armazenar a resposta do sensor

// Comando para ler 7 registradores a partir do 0x0000 (função Modbus 0x03)
const byte lerTodosOsSensores[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08 };

void setup() {
  Serial.begin(115200);
  mod.begin(4800, SERIAL_8N1, 16, 17); // RX=16 (RO), TX=17 (DI)

  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  digitalWrite(RE, LOW);
  digitalWrite(DE, LOW);

  Serial.println("Leitor NPK PHCTH-S 7 em 1 via RS485 (ESP32)");
  delay(500);
}

void loop() {
  LerSensores();

  // Armazena os valores convertidos
  unsigned int umidade, temperatura, condutividade, ph, nitrogenio, fosforo, potassio;

  // Conversões
  umidade      = (values[3] << 8) | values[4];
  temperatura  = (values[5] << 8) | values[6];
  condutividade= (values[7] << 8) | values[8];
  ph           = (values[9] << 8) | values[10];
  nitrogenio   = (values[11] << 8) | values[12];
  fosforo      = (values[13] << 8) | values[14];
  potassio     = (values[15] << 8) | values[16];

  // Impressão
  Serial.println("========== Leitura do Sensor ==========");
  Serial.print("|| Umidade: ");       Serial.print(umidade / 10.0);     Serial.println(" %");
  Serial.print("|| Temperatura: ");   Serial.print(temperatura / 10.0); Serial.println(" °C");
  Serial.print("|| Condutividade: "); Serial.print(condutividade);      Serial.println(" uS/cm");
  Serial.print("|| pH: ");            Serial.println(ph / 10.0);
  Serial.print("|| Nitrogênio (N): ");Serial.print(nitrogenio);         Serial.println(" mg/kg");
  Serial.print("|| Fósforo (P): ");   Serial.print(fosforo);             Serial.println(" mg/kg");
  Serial.print("|| Potássio (K): ");  Serial.print(potassio);            Serial.println(" mg/kg");
  Serial.println("=======================================\n");
  Serial.println();

  delay(5000);  // Espera 5 segundos
}

void LerSensores() {
  // Habilita transmissão RS485
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(2);

  // Envia comando
  mod.write(lerTodosOsSensores, sizeof(lerTodosOsSensores));
  mod.flush();  // Aguarda envio

  // Habilita recepção
  digitalWrite(DE, LOW);
  digitalWrite(RE, LOW);

  delay(100); // Espera a resposta chegar

  byte i = 0;
  while (mod.available() && i < sizeof(values)) {
    values[i] = mod.read();
    printHexByte(values[i]);
    i++;
  }
  Serial.println();
}

// Imprime byte em hexadecimal
void printHexByte(byte b) {
  if (b < 0x10) Serial.print("0");
  Serial.print(b, HEX);
  Serial.print(' ');
}
