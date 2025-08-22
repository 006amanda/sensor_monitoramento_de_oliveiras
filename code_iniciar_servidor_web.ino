#include <WiFi.h>
#include <WebServer.h>

// Cria servidor na porta 80
WebServer server(80);

// Função que responde no navegador
void handleRoot() {
  server.send(200, "text/html", "<h1>ESP32 Web Server OK 🥷🏻</h1>");
}

void setup() {
  Serial.begin(115200);

  // Inicia Access Point
  WiFi.softAP("ESP32_ACCESS POINT", "amanda2006");
  Serial.println("Access Point iniciado!");
  Serial.print("IP do ESP32: ");
  Serial.println(WiFi.softAPIP());

  // Define rota principal "/"
  server.on("/", handleRoot);

  // Inicia servidor web
  server.begin();
  Serial.println("Servidor web iniciado!");
}

void loop() {
  server.handleClient();  // mantém servidor respondendo
}
