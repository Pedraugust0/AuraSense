#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// Variaveis de Conexao AP (Access Point)
const char* ap_ssid = "ConfigSensor";         // Nome da rede Wi-Fi que o ESP32 vai criar
const char* ap_password = "";                 // Senha para se conectar ao AP do ESP32
IPAddress apIP(192, 168, 4, 1);               // IP padrao do ESP32 quando ele e um AP

DNSServer dnsServer;                          // Servidor DNS para redirecionamento automatico
WebServer webServer(80);                      // Servidor WEB para servir as páginas

// Pinos do hardware
int sensorPin = 39;
int ledPin = 27;
int buzzerPin = 32;

// Variaveis de Config
int buzzerTone = 500;
int limiteGas = 700;

int valorSensor;
bool alarmeAtivo = false;

// PAGINAS HTML
const char* HTML_HOME_PAGE = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Config. Sensor Gas</title>
    <style>
        body{font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #e0f7fa;}
        h1{color: #00796b;}
        form{background-color: #ffffff; margin: 20px auto; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 400px;}
        label{display: block; margin-bottom: 5px; color: #333;}
        input[type="number"] {width: calc(100% - 22px); padding: 10px; margin-bottom: 10px; border: 1px solid #ddd; border-radius: 4px;}
        input[type="submit"] {background-color: #00796b; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px;}
        input[type="submit"]:hover {background-color: #004d40;}
        p{color: #555;}
    </style>
</head>
<body>
    <h1>Configuracoes do Sensor de Gas</h1>

    <form action="/update_settings" method="post">
        <label for="limiteGas">Limite do Sensor de Gas (0-4000):</label>
        <input type="number" id="limiteGas" name="limiteGas" min="0" max="4000" value="%LIMITE_GAS%"><br><br>

        <label for="buzzerTone">Frequencia do Buzzer (Hz):</label>
        <input type="number" id="buzzerTone" name="buzzerTone" min="200" max="2000" value="%BUZZER_TONE%"><br><br>

        <input type="submit" value="Salvar Configuracoes">
    </form>
</body>
</html>
)rawliteral";


// FUNCOES DO SERVIDOR WEB

void handleRoot() {
  String html = HTML_HOME_PAGE;
  html.replace("%LIMITE_GAS%", String(limiteGas));
  html.replace("%BUZZER_TONE%", String(buzzerTone));
  webServer.send(200, "text/html", html);
}

void handleUpdateSettings() {
  if (webServer.hasArg("limiteGas")) {
    limiteGas = webServer.arg("limiteGas").toInt();
    if (limiteGas < 0) limiteGas = 0;
    if (limiteGas > 4000) limiteGas = 4000;
  }
  if (webServer.hasArg("buzzerTone")) {
    buzzerTone = webServer.arg("buzzerTone").toInt();
    if (buzzerTone < 200) buzzerTone = 200;
    if (buzzerTone > 2000) buzzerTone = 2000;
  }

  webServer.sendHeader("Location", "/", true);
  webServer.send(302, "text/plain", ""); // Redireciona para a pagina inicial
}

void handleNotFound() {
  webServer.send(404, "text/plain", "Nao encontrado");
}

// Configurar o ESP como AP
void startWiFiAP() {
  WiFi.mode(WIFI_AP); // Define para operar em Access Point
  WiFi.softAP(ap_ssid, ap_password); // Inicia

  Serial.print("AP iniciado: ");
  Serial.print(ap_ssid);
  Serial.print(" IP: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(53, "*", apIP); // Inicia o servidor DNS para redirecionamento
}

// SETUP e LOOP principais
void setup() {
  pinMode(sensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);

  startWiFiAP();

  // Lida com as paginas
  webServer.on("/", handleRoot);
  webServer.on("/update_settings", HTTP_POST, handleUpdateSettings);
  webServer.onNotFound(handleNotFound);

  webServer.begin();
  Serial.println("Servidor Web iniciado.");
}

void loop() {
  dnsServer.processNextRequest(); // Processa requisicoes DNS para o captive portal
  webServer.handleClient();       // Processa as requisicoes HTTP

  int valorSensor = analogRead(sensorPin);

  if (valorSensor > limiteGas && !alarmeAtivo) {
      digitalWrite(ledPin, HIGH);
      tone(buzzerPin, buzzerTone);
      alarmeAtivo = true;
      Serial.println("Alarme ATIVADO!");
  } else if (valorSensor <= (limiteGas - 50) && alarmeAtivo) {
      digitalWrite(ledPin, LOW);
      noTone(buzzerPin);
      alarmeAtivo = false;
      Serial.println("Alarme DESATIVADO!");
  }

  // Serial prints para debug
  Serial.print("Sensor: ");
  Serial.print(valorSensor);
  Serial.print(" Limite: ");
  Serial.print(limiteGas);
  Serial.print(" Alarme Ativo: ");
  Serial.println(alarmeAtivo ? "SIM" : "NAO");
}
