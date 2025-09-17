#include <WiFi.h>
#include <WebServer.h>

// ====== CONFIG AP (NÃO deixe senhas fixas em produção) ======
const char* ssid     = "ESP32_ACCESS_POINT";
const char* password = "amanda2006";

WebServer server(80);

// ====== PINOS ======
const int PINO_UMIDADE = 34;  // AO -> GPIO34 (somente entrada)
const int PINO_RELE    = 22;  // Relé (confira se é ativo baixo)

// ====== CALIBRAÇÃO ======
// Ajuste conforme seu sensor/solo:
int adc_seco    = 3500;  // leitura no ar/solo seco
int adc_molhado = 1200;  // leitura com solo bem úmido

// ====== CONTROLE ======
volatile int  umidade_percent = 0;
volatile bool bombaLigada     = false;

// Modo automático x manual
bool modoAutomatico = true;
int  limiteLiga     = 35;  // se umidade < 35% -> liga
int  limiteDesliga  = 55;  // se umidade > 55% -> desliga (histerese)

// Suavização (média móvel exponencial)
float filtroEMA = 0.15f;
float leituraSuavizada = 0;

// Timers (loop não-bloqueante)
unsigned long tAgora = 0;
unsigned long tLeituraPrev = 0;
const unsigned long intervaloLeituraMs = 1000;

unsigned long tUIprev = 0;
const unsigned long intervaloUIms = 250; // servir clientes e atualizar rápido

// ====== HTML (frontend com AJAX p/ /status) ======
String paginaHTML() {
  String html = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<title>Monitor de Irrigação</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { font-family: system-ui, Arial, sans-serif; background:#f6f7fb; margin:0; padding:24px; color:#222 }
  .wrap { max-width: 560px; margin: 0 auto; }
  .card { background:#fff; border-radius:16px; padding:20px; box-shadow:0 8px 24px rgba(0,0,0,.08); }
  h1 { margin:0 0 12px; font-size:1.25rem }
  .linha { display:flex; justify-content:space-between; align-items:center; margin:8px 0 }
  .valor { font-size:2.25rem; font-weight:700 }
  .ok { color:#28a745 } .nok { color:#dc3545 }
  .btns { display:flex; gap:8px; flex-wrap:wrap; margin-top:14px }
  button { border:0; border-radius:10px; padding:10px 14px; font-size:1rem; cursor:pointer }
  .ligar { background:#28a745; color:#fff }
  .desligar { background:#dc3545; color:#fff }
  .alt { background:#0d6efd; color:#fff }
  .row { display:grid; grid-template-columns:1fr 1fr; gap:8px; margin-top:8px }
  input[type=number]{ width:100%; padding:8px 10px; border:1px solid #ddd; border-radius:8px }
  .badge { padding:4px 10px; border-radius:999px; background:#eee; font-size:.9rem }
</style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>🌱 Sistema de Irrigação ESP32</h1>
      <div class="linha">
        <div><strong>Umidade do Solo</strong></div>
        <div class="valor" id="umidade">--%</div>
      </div>
      <div class="linha">
        <div><strong>Bomba</strong></div>
        <div id="bomba" class="badge">--</div>
      </div>
      <div class="linha">
        <div><strong>Modo</strong></div>
        <div id="modo" class="badge">--</div>
      </div>

      <div class="btns">
        <form action="/ligar"><button class="ligar">Ligar (Manual)</button></form>
        <form action="/desligar"><button class="desligar">Desligar (Manual)</button></form>
        <form action="/auto_on"><button class="alt">Modo Automático</button></form>
        <form action="/auto_off"><button class="alt" style="background:#6c757d">Modo Manual</button></form>
      </div>

      <div class="row">
        <form action="/set_limites" method="GET">
          <div style="margin-top:10px"><strong>Limites (Auto)</strong></div>
          <div class="row">
            <div>
              <label>Ligar abaixo de (%)</label>
              <input type="number" name="liga" min="0" max="100" required>
            </div>
            <div>
              <label>Desligar acima de (%)</label>
              <input type="number" name="desliga" min="0" max="100" required>
            </div>
          </div>
          <button class="alt" style="margin-top:8px">Aplicar Limites</button>
        </form>
      </div>
    </div>
  </div>

<script>
async function atualiza() {
  try{
    const r = await fetch('/status');
    const s = await r.json();
    document.getElementById('umidade').textContent = s.umidade + '%';
    document.getElementById('bomba').textContent = s.bomba ? '💧 Ligada' : '❌ Desligada';
    document.getElementById('bomba').className = 'badge ' + (s.bomba ? 'ok':'nok');
    document.getElementById('modo').textContent = s.automatico ? 'Automático' : 'Manual';
  }catch(e){}
}
setInterval(atualiza, 1000);
atualiza();
</script>
</body>
</html>)rawliteral";
  return html;
}

// ====== HELPERS ======
int adcParaPercentual(int adc) {
  // mapeia seco->0% e molhado->100% (inverte se necessário)
  int pct = map(adc, adc_seco, adc_molhado, 0, 100);
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

void ligaBomba(bool liga) {
  // Atenção: muitos módulos de relé são ATIVOS EM LOW.
  // Se o seu for ativo em LOW, troque HIGH/LOW aqui.
  digitalWrite(PINO_RELE, liga ? HIGH : LOW);
  bombaLigada = liga;
}

// ====== HANDLERS HTTP ======
void handleRoot() { server.send(200, "text/html", paginaHTML()); }

void handleLigar()     { modoAutomatico = false; ligaBomba(true);  server.sendHeader("Location","/"); server.send(303); }
void handleDesligar()  { modoAutomatico = false; ligaBomba(false); server.sendHeader("Location","/"); server.send(303); }
void handleAutoOn()    { modoAutomatico = true;  server.sendHeader("Location","/"); server.send(303); }
void handleAutoOff()   { modoAutomatico = false; server.sendHeader("Location","/"); server.send(303); }

void handleSetLimites() {
  if (server.hasArg("liga") && server.hasArg("desliga")) {
    int l1 = server.arg("liga").toInt();
    int l2 = server.arg("desliga").toInt();
    // garante histerese coerente
    if (l1 >= 0 && l2 <= 100 && l1 < l2) {
      limiteLiga = l1;
      limiteDesliga = l2;
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStatus() {
  String json = "{";
  json += "\"umidade\":" + String(umidade_percent) + ",";
  json += "\"bomba\":"   + String(bombaLigada ? 1 : 0) + ",";
  json += "\"automatico\":" + String(modoAutomatico ? 1 : 0) + ",";
  json += "\"liga\":" + String(limiteLiga) + ",";
  json += "\"desliga\":" + String(limiteDesliga);
  json += "}";
  server.send(200, "application/json", json);
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);

  pinMode(PINO_RELE, OUTPUT);
  ligaBomba(false); // começa desligada

  // melhora faixa do ADC para sensores (opcional)
  analogSetPinAttenuation(PINO_UMIDADE, ADC_11db); // 0-3.6V

  WiFi.softAP(ssid, password);
  Serial.println("AP iniciado!");
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  server.on("/",          handleRoot);
  server.on("/ligar",     handleLigar);
  server.on("/desligar",  handleDesligar);
  server.on("/auto_on",   handleAutoOn);
  server.on("/auto_off",  handleAutoOff);
  server.on("/set_limites", handleSetLimites);
  server.on("/status",    handleStatus);

  server.begin();
}

// ====== LOOP ======
void loop() {
  tAgora = millis();
  server.handleClient();

  // Leitura e processamento ~1s
  if (tAgora - tLeituraPrev >= intervaloLeituraMs) {
    tLeituraPrev = tAgora;

    int adc = analogRead(PINO_UMIDADE);

    // suavização
    if (leituraSuavizada == 0) leituraSuavizada = adc; // primeira amostra
    leituraSuavizada = (1.0f - filtroEMA) * leituraSuavizada + filtroEMA * adc;

    umidade_percent = adcParaPercentual((int)leituraSuavizada);

    // Controle automático com histerese
    if (modoAutomatico) {
      if (umidade_percent < limiteLiga && !bombaLigada) {
        ligaBomba(true);
      } else if (umidade_percent > limiteDesliga && bombaLigada) {
        ligaBomba(false);
      }
    }

    Serial.printf("ADC: %d | Filtrado: %.1f | Umidade: %d%% | Bomba: %s | Modo: %s\n",
                  adc, leituraSuavizada, umidade_percent,
                  bombaLigada ? "ON" : "OFF",
                  modoAutomatico ? "AUTO" : "MANUAL");
  }

  // pequeno yield implícito pelo loop não-bloqueante
}
