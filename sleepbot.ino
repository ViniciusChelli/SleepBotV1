// SleepBot V1 - Versão ESP32 com painel web dinâmico com gráficos avançados, dark mode e mais sensores
// Autor: Vinícius Chelli + ChatGPT

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>

#define DHTPIN 15
#define DHTTYPE DHT11
#define LDRPIN 36
#define SOUND_PIN 39
#define MQ2_PIN 34
#define PIR_PIN 14
#define VIB_PIN 35
#define LED_VERDE 27
#define LED_AMARELO 26
#define LED_VERMELHO 25
#define BUZZER_PIN 13

DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
AsyncWebServer server(80);

const char* ssid = "pix bladehelios@gmail.com";
const char* password = "Pa250250700@@@@@@@";

String getSensorData() {
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  int luz = analogRead(LDRPIN);
  int som = analogRead(SOUND_PIN);
  int gas = analogRead(MQ2_PIN);
  int mov = digitalRead(PIR_PIN);
  int vib = analogRead(VIB_PIN);
  char buf[300];
  sprintf(buf, "{\"temp\":%.1f,\"umid\":%.1f,\"luz\":%d,\"som\":%d,\"gas\":%d,\"mov\":%d,\"vib\":%d}",
          temp, umid, luz, som, gas, mov, vib);
  return String(buf);
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <title>SleepBot - Monitoramento</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body {
      font-family: 'Segoe UI', sans-serif;
      background: #121212;
      color: #eee;
      text-align: center;
      padding: 20px;
    }
    h1 {
      color: #66bb6a;
    }
    canvas {
      background: #1f1f1f;
      border-radius: 8px;
      margin: 20px auto;
      max-width: 90%;
    }
    .cards {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
    }
    .card {
      background: #2a2a2a;
      border-radius: 12px;
      padding: 10px 20px;
      margin: 10px;
      min-width: 140px;
      color: #81c784;
      box-shadow: 0 0 10px #000;
    }
    footer {
      margin-top: 40px;
      font-size: 12px;
      color: #777;
    }
  </style>
</head>
<body>
  <h1>SleepBot</h1>
  <p>Monitoramento ambiental em tempo real com gráficos</p>

  <div class="cards">
    <div class="card"><strong>Temperatura:</strong> <span id="temp">--</span> °C</div>
    <div class="card"><strong>Umidade:</strong> <span id="umid">--</span> %</div>
    <div class="card"><strong>Luz:</strong> <span id="luz">--</span></div>
    <div class="card"><strong>Som:</strong> <span id="som">--</span></div>
    <div class="card"><strong>Gás:</strong> <span id="gas">--</span></div>
    <div class="card"><strong>Movimento:</strong> <span id="mov">--</span></div>
    <div class="card"><strong>Vibração:</strong> <span id="vib">--</span></div>
  </div>

  <canvas id="tempChart"></canvas>
  <canvas id="umidChart"></canvas>
  <canvas id="luzChart"></canvas>
  <canvas id="somChart"></canvas>
  <canvas id="gasChart"></canvas>
  <canvas id="vibChart"></canvas>

  <footer>Desenvolvido por Vinícius Chelli - SleepBot v1</footer>

  <script>
    const createChart = (ctxId, label, color, suggestedMin, suggestedMax) => {
      return new Chart(document.getElementById(ctxId).getContext('2d'), {
        type: 'line',
        data: {
          labels: [],
          datasets: [{
            label: label,
            data: [],
            borderColor: color,
            backgroundColor: color + '33',
            fill: true,
            tension: 0.3
          }]
        },
        options: {
          scales: {
            x: { display: false },
            y: {
              beginAtZero: true,
              suggestedMin: suggestedMin,
              suggestedMax: suggestedMax
            }
          }
        }
      });
    };

    const tempChart = createChart("tempChart", "Temperatura (°C)", "#66bb6a", 15, 40);
    const umidChart = createChart("umidChart", "Umidade (%)", "#29b6f6", 0, 100);
    const luzChart = createChart("luzChart", "Luz (ADC)", "#ffa726", 0, 1024);
    const somChart = createChart("somChart", "Som (ADC)", "#ab47bc", 0, 1024);
    const gasChart = createChart("gasChart", "Gás (ADC)", "#ef5350", 0, 1024);
    const vibChart = createChart("vibChart", "Vibração (ADC)", "#26a69a", 0, 1024);

    setInterval(() => {
      fetch("/dados")
        .then(res => res.json())
        .then(data => {
          const time = new Date().toLocaleTimeString();
          document.getElementById("temp").innerText = data.temp;
          document.getElementById("umid").innerText = data.umid;
          document.getElementById("luz").innerText = data.luz;
          document.getElementById("som").innerText = data.som;
          document.getElementById("gas").innerText = data.gas;
          document.getElementById("mov").innerText = data.mov ? "Detectado" : "Ausente";
          document.getElementById("vib").innerText = data.vib;

          const charts = [
            [tempChart, data.temp],
            [umidChart, data.umid],
            [luzChart, data.luz],
            [somChart, data.som],
            [gasChart, data.gas],
            [vibChart, data.vib]
          ];

          charts.forEach(([chart, value]) => {
            if (chart.data.labels.length > 20) {
              chart.data.labels.shift();
              chart.data.datasets[0].data.shift();
            }
            chart.data.labels.push(time);
            chart.data.datasets[0].data.push(value);
            chart.update();
          });
        });
    }, 2000);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  dht.begin();
  rtc.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/dados", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", getSensorData());
  });

  server.begin();
}

void loop() {
  // Servidor assíncrono - nada necessário aqui
}
