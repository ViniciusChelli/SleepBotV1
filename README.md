# SleepBotV1
This project apresents the sleep bot version 1, an idea that surrounds in monitoring your sleep with multiple sensors and providing information about it.
SleepBot V1 - Sistema Inteligente de Monitoramento Noturno

Desenvolvido por Vinícius Chelli, o projeto SleepBot V1 é um sistema de monitoramento ambiental completo com ESP32, voltado para acompanhamento da qualidade do ambiente durante o sono. Ele coleta dados em tempo real e exibe de forma visual e responsiva via Wi-Fi local.

🌟 Destaques do Projeto

Interface Web com dark mode e gráficos dinâmicos via Chart.js

Monitoramento de:

Temperatura e Umidade (DHT11)

Luminosidade (LDR)

Som ambiente (microfone analógico)

Qualidade do ar (MQ-135)

Presença/Movimento (PIR HC-SR501)

Vibração (SW-420)

RTC (relógio em tempo real - DS3231)

LED indicators e buzzer para alarmes

⚖️ Componentes Utilizados

ESP32 30 pinos

DHT11

LDR + resistor de 10k

Sensor de som (analógico)

Sensor de gás MQ-135

Sensor PIR HC-SR501

Sensor de vibração SW-420

RTC DS3231

Display LCD 16x2 com I2C

Teclado matricial 4x4

3 LEDs (verde, amarelo, vermelho)

Buzzer ativo 5V

Protoboard de 1660 pontos

⚙️ Funcionalidades

Coleta periódica de dados a cada 2 segundos

Exibição dos dados em gráficos de linha

Atualização via AJAX sem recarregar a página

Indicadores visuais de estado (detectado/ausente)

Integração simples via Wi-Fi local (sem necessidade de internet)

🔧 Instalação

Abra a Arduino IDE

Instale as seguintes bibliotecas:

ESPAsyncWebServer

AsyncTCP

DHT sensor library

Adafruit Sensor

RTClib

Instale a placa ESP32 nas preferências da IDE

Substitua ssid e password no código pelo nome e senha da sua rede Wi-Fi

Conecte os sensores conforme o esquema e faça upload do código

🔌 Acesso ao Painel

Verifique o IP local da ESP32 no monitor serial

Acesse esse IP no navegador da mesma rede (ex: http://192.168.1.100)

✏️ Futuras Expansões

Log de dados em SD Card ou envio via Telegram

Modo IA para detecção de padrões de sono

Integração com apps de celular ou dashboards online

Vinícius Chelli - 2025Contato: theongatechelli@gmail.com

