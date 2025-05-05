// SleepBot V1 - com ESP32, Web UI e MAX30100
// Autor: Vinícius Chelli + ChatGPT

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include "MAX30100_PulseOximeter.h"

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

#define REPORTING_PERIOD_MS 1000

DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
PulseOximeter pox;
AsyncWebServer server(80);

const char* ssid = "pix bladehelios@gmail.com";
const char* password = "Pa250250700@@@@@@@";

uint32_t tsLastReport = 0;
float bpm = 0, spo2 = 0;

String getSensorData() {
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  int luz = analogRead(LDRPIN);
  int som = analogRead(SOUND_PIN);
  int gas = analogRead(MQ2_PIN);
  int mov = digitalRead(PIR_PIN);
  int vib = analogRead(VIB_PIN);

  char buf[500];
  sprintf(buf, "{\"temp\":%.1f,\"umid\":%.1f,\"luz\":%d,\"som\":%d,\"gas\":%d,\"mov\":%d,\"vib\":%d,\"bpm\":%.1f,\"spo2\":%.1f}",
          temp, umid, luz, som, gas, mov, vib, bpm, spo2);
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
    h1 { color: #66bb6a; }
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
    <div class="card"><strong>Temp:</strong> <span id="temp">--</span> °C</div>
    <div class="card"><strong>Umidade:</strong> <span id="umid">--</span> %</div>
    <div class="card"><strong>Luz:</strong> <span id="luz">--</span></div>
    <div class="card"><strong>Som:</strong> <span id="som">--</span></div>
    <div class="card"><strong>Gás:</strong> <span id="gas">--</span></div>
    <div class="card"><strong>Movimento:</strong> <span id="mov">--</span></div>
    <div class="card"><strong>Vibração:</strong> <span id="vib">--</span></div>
    <div class="card"><strong>BPM:</strong> <span id="bpm">--</span></div>
    <div class="card"><strong>SpO₂:</strong> <span id="spo2">--</span> %</div>
  </div>

  <canvas id="bpmChart"></canvas>

  <footer>Desenvolvido por Vinícius Chelli - SleepBot v1</footer>

  <script>
    const bpmChart = new Chart(document.getElementById("bpmChart").getContext("2d"), {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          label: "Batimentos (BPM)",
          data: [],
          borderColor: "#f44336",
          backgroundColor: "#f4433633",
          fill: true,
          tension: 0.3
        }]
      },
      options: {
        scales: {
          x: { display: false },
          y: { beginAtZero: true, suggestedMin: 40, suggestedMax: 140 }
        }
      }
    });

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
          document.getElementById("bpm").innerText = data.bpm.toFixed(1);
          document.getElementById("spo2").innerText = data.spo2.toFixed(1);

          if (bpmChart.data.labels.length > 20) {
            bpmChart.data.labels.shift();
            bpmChart.data.datasets[0].data.shift();
          }
          bpmChart.data.labels.push(time);
          bpmChart.data.datasets[0].data.push(data.bpm);
          bpmChart.update();
        });
    }, 2000);
  </script>
</body>
</html>
)rawliteral";

void onPulseDetected() {
  Serial.println("Pulso detectado!");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  rtc.begin();
  Wire.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  if (!pox.begin()) {
    Serial.println("Erro ao iniciar o MAX30100");
  } else {
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6mA);
    pox.setOnBeatDetectedCallback(onPulseDetected);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
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
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    bpm = pox.getHeartRate();
    spo2 = pox.getSpO2();
    tsLastReport = millis();
  }
}
