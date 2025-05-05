// SleepBot V1 – ESP32 (38 pinos), Web UI, MAX30100, Chaves Alavanca
// Autor: Vinícius Chelli + ChatGPT

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include "MAX30100_PulseOximeter.h"

// Sensores
#define DHTPIN 15
#define DHTTYPE DHT11
#define LDRPIN 36
#define SOUND_PIN 39
#define MQ2_PIN 34
#define PIR_PIN 14
#define VIB_PIN 35

// LEDs e Buzzer
#define LED_VERDE 21
#define LED_AMARELO 22
#define LED_VERMELHO 23
#define BUZZER_PIN 13

// Botões (chaves alavanca)
#define CHAVE_SENSOR_EXTRA 32
#define CHAVE_SILENCIO     33
#define CHAVE_WEBSERVER    25
#define CHAVE_LOG          26
#define CHAVE_ALARME       27

// Comunicação I2C
#define SDA_PIN 18
#define SCL_PIN 19

// Wi-Fi
const char* ssid = "pix bladehelios@gmail.com";
const char* password = "Pa250250700@@@@@@@";

#define REPORTING_PERIOD_MS 1000

DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;
PulseOximeter pox;
AsyncWebServer server(80);

float bpm = 0, spo2 = 0;
uint32_t tsLastReport = 0;

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
  <p>Monitoramento ambiental e biométrico em tempo real</p>

  <div class="cards">
    <div class="card"><strong>Temperatura:</strong> <span id="temp">--</span> °C</div>
    <div class="card"><strong>Umidade:</strong> <span id="umid">--</span> %</div>
    <div class="card"><strong>Luz:</strong> <span id="luz">--</span></div>
    <div class="card"><strong>Som:</strong> <span id="som">--</span></div>
    <div class="card"><strong>Gás:</strong> <span id="gas">--</span></div>
    <div class="card"><strong>Movimento:</strong> <span id="mov">--</span></div>
    <div class="card"><strong>Vibração:</strong> <span id="vib">--</span></div>
    <div class="card"><strong>BPM:</strong> <span id="bpm">--</span></div>
    <div class="card"><strong>SpO₂:</strong> <span id="spo2">--</span> %</div>
  </div>

  <canvas id="tempChart"></canvas>
  <canvas id="umidChart"></canvas>
  <canvas id="luzChart"></canvas>
  <canvas id="somChart"></canvas>
  <canvas id="gasChart"></canvas>
  <canvas id="vibChart"></canvas>
  <canvas id="bpmChart"></canvas>
  <canvas id="spo2Chart"></canvas>

  <footer>Desenvolvido por Vinícius Chelli - SleepBot v1</footer>

  <script>
    const createChart = (ctxId, label, color, min, max) => {
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
              suggestedMin: min,
              suggestedMax: max
            }
          }
        }
      });
    };

    const charts = {
      temp: createChart("tempChart", "Temperatura (°C)", "#66bb6a", 10, 40),
      umid: createChart("umidChart", "Umidade (%)", "#29b6f6", 0, 100),
      luz: createChart("luzChart", "Luz (ADC)", "#ffa726", 0, 1024),
      som: createChart("somChart", "Som (ADC)", "#ab47bc", 0, 1024),
      gas: createChart("gasChart", "Gás (ADC)", "#ef5350", 0, 1024),
      vib: createChart("vibChart", "Vibração (ADC)", "#26a69a", 0, 1024),
      bpm: createChart("bpmChart", "BPM", "#8e24aa", 40, 140),
      spo2: createChart("spo2Chart", "SpO₂ (%)", "#42a5f5", 85, 100)
    };

    function updateCard(id, value, unit = "") {
      document.getElementById(id).innerText = value + unit;
    }

    setInterval(() => {
      fetch("/dados")
        .then(res => res.json())
        .then(data => {
          const time = new Date().toLocaleTimeString();

          updateCard("temp", data.temp, " °C");
          updateCard("umid", data.umid, " %");
          updateCard("luz", data.luz);
          updateCard("som", data.som);
          updateCard("gas", data.gas);
          updateCard("mov", data.mov ? "Detectado" : "Ausente");
          updateCard("vib", data.vib);
          updateCard("bpm", data.bpm);
          updateCard("spo2", data.spo2, " %");

          for (let key in charts) {
            const chart = charts[key];
            const value = data[key];
            if (chart.data.labels.length > 20) {
              chart.data.labels.shift();
              chart.data.datasets[0].data.shift();
            }
            chart.data.labels.push(time);
            chart.data.datasets[0].data.push(value);
            chart.update();
          }
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
  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(CHAVE_SENSOR_EXTRA, INPUT_PULLUP);
  pinMode(CHAVE_SILENCIO, INPUT_PULLUP);
  pinMode(CHAVE_WEBSERVER, INPUT_PULLUP);
  pinMode(CHAVE_LOG, INPUT_PULLUP);
  pinMode(CHAVE_ALARME, INPUT_PULLUP);

  if (!pox.begin()) {
    Serial.println("Erro ao iniciar MAX30100");
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

  if (digitalRead(CHAVE_WEBSERVER) == LOW) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });
    server.on("/dados", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "application/json", getSensorData());
    });
    server.begin();
  }
}

void loop() {
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    bpm = pox.getHeartRate();
    spo2 = pox.getSpO2();
    tsLastReport = millis();
  }

  if (digitalRead(CHAVE_SILENCIO) == LOW) {
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARELO, LOW);
    digitalWrite(LED_VERMELHO, LOW);
  }
}
