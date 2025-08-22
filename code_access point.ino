#include <WiFi.h>

void setup() {
  Serial.begin(115200);

  // Cria Access Point com SSID e senha
  const char* ssid = "ESP32_ACCESS POINT";
  const char* password = "amanda2006";

  WiFi.softAP(ssid, password);

  Serial.println("Access Point iniciado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.softAPIP());  // Normalmente 192.168.4.1
}

void loop() {
  // Nada no loop, só mantém o AP
}
