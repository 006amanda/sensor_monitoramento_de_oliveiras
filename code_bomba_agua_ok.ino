#include <WiFi.h>
#include <WebServer.h>

// === CONFIGURAÇÃO DO ACCESS POINT ===
const char* ssid     = "ESP32_ACCESS POINT";
const char* password = "amanda2006";

WebServer server(80);

// === PINOS ===
const int PINO_UMIDADE = 34;  // Sensor AO -> GPIO34
const int PINO_RELE    = 22;  // Relé IN2 -> GPIO22

// === CONFIGURAÇÃO RELÉ ===
// Se seu módulo de relé é ativo em LOW (a maioria), deixe true.
bool RELE_ATIVO_LOW = true;

// === VARIÁVEIS ===
int umidade = 0;
bool bombaLigada = false;
bool modoAutomatico = false;

// Limites do automático (histerese)
int limiteLiga = 35;   // Liga quando umidade < 35%
int limiteDesliga = 55; // Desliga quando umidade > 55%

// === FUNÇÕES AUXILIARES ===
void writeRele(bool liga) {
  if (RELE_ATIVO_LOW) {
    digitalWrite(PINO_RELE, liga ? LOW : HIGH);
  } else {
    digitalWrite(PINO_RELE, liga ? HIGH : LOW);
  }
  bombaLigada = liga;
}

String statusBomba() {
  return bombaLigada ? "💧 Ligada" : "❌ Desligada";
}

String statusModo() {
  return modoAutomatico ? "Automático" : "Manual";
}

// === PÁGINA HTML ===
String paginaHTML() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <title>Monitor de Irrigação</title>
    <style>
      body { font-family: Arial, sans-serif; background: #f2f2f2; text-align: center; }
      h1 { color: #333; }
      .card {
        background: white; padding: 20px; margin: 20px auto; width: 300px;
        border-radius: 15px; box-shadow: 0 4px 8px rgba(0,0,0,0.2);
      }
      .valor { font-size: 2em; margin: 10px 0; color: #28a745; }
      button {
        padding: 10px 20px; font-size: 1em;
        border: none; border-radius: 10px; cursor: pointer;
        margin: 10px;
      }
      .ligar { background: #28a745; color: white; }
      .desligar { background: #dc3545; color: white; }
      .auto { background: #007bff; color: white; }
    </style>
  </head>
  <body>
    <h1>🌱 Sistema de Irrigação ESP32</h1>
    <div class="card">
      <p><strong>Umidade do Solo:</strong></p>
      <p class="valor">)rawliteral" + String(umidade) + R"rawliteral(%</p>
      <p><strong>Bomba:</strong> )rawliteral" + statusBomba() + R"rawliteral(</p>
      <p><strong>Modo:</strong> )rawliteral" + statusModo() + R"rawliteral(</p>
      <form action="/ligar"><button class="ligar">Ligar (Manual)</button></form>
      <form action="/desligar"><button class="desligar">Desligar (Manual)</button></form>
      <form action="/auto"><button class="auto">Ativar Automático</button></form>
    </div>
  </body>
  </html>
  )rawliteral";

  return html;
}

// === HANDLERS ===
void handleRoot() {
  server.send(200, "text/html", paginaHTML());
}

void handleLigar() {
  modoAutomatico = false;
  writeRele(true);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDesligar() {
  modoAutomatico = false;
  writeRele(false);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleAuto() {
  modoAutomatico = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

// === SETUP ===
void setup() {
  Serial.begin(115200);

  pinMode(PINO_RELE, OUTPUT);
  writeRele(false); // Começa desligada

  analogSetPinAttenuation(PINO_UMIDADE, ADC_11db);

  WiFi.softAP(ssid, password);
  Serial.println("Access Point iniciado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/ligar", handleLigar);
  server.on("/desligar", handleDesligar);
  server.on("/auto", handleAuto);

  server.begin();
}

// === LOOP ===
void loop() {
  server.handleClient();

  // Lê umidade a cada 1s
  static unsigned long ultimo = 0;
  if (millis() - ultimo >= 1000) {
    ultimo = millis();

    int leitura = analogRead(PINO_UMIDADE);

    int seco = 3500;
    int molhado = 1200;

    umidade = map(leitura, seco, molhado, 0, 100);
    if (umidade < 0) umidade = 0;
    if (umidade > 100) umidade = 100;

    Serial.print("Leitura bruta: ");
    Serial.print(leitura);
    Serial.print(" | Umidade: ");
    Serial.print(umidade);
    Serial.print("% | Bomba: ");
    Serial.print(bombaLigada ? "ON" : "OFF");
    Serial.print(" | Modo: ");
    Serial.println(modoAutomatico ? "AUTO" : "MANUAL");

    // Controle automático
    if (modoAutomatico) {
      if (umidade < limiteLiga && !bombaLigada) {
        writeRele(true);
        Serial.println(">> AUTOMÁTICO: Ligando bomba");
      } else if (umidade > limiteDesliga && bombaLigada) {
        writeRele(false);
        Serial.println(">> AUTOMÁTICO: Desligando bomba");
      }
    }
  }
}
