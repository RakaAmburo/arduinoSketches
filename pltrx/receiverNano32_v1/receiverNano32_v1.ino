#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "secrets.h"

#define BUZZER_PIN  5
#define MY_CMD      'B'
#define MQTT_BROKER "192.168.1.135"
#define MQTT_PORT   1883
#define TOPIC_CMD   "pltrx/receiver/cmd"
#define TOPIC_LOG   "pltrx/receiver/log"

#define PL_PREAMBLE 0xAA
#define PL_SUFFIX   0xFF

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

void mqttLog(const char* msg) {
  Serial.println(msg);
  mqtt.publish(TOPIC_LOG, msg);
}

void sendAck() {
  Serial1.write((byte)PL_PREAMBLE);
  Serial1.write((byte)'K');
  Serial1.write((byte)PL_SUFFIX);
}

void handlePowerline() {
  static byte state = 0;
  static byte cmdByte = 0;

  while (Serial1.available()) {
    byte b = Serial1.read();
    switch (state) {
      case 0:
        if (b == PL_PREAMBLE) state = 1;
        break;
      case 1:
        cmdByte = b;
        state = 2;
        break;
      case 2:
        if (b == PL_SUFFIX) {
          char logbuf[32];
          snprintf(logbuf, sizeof(logbuf), "recibido cmd: %c", cmdByte);
          mqttLog(logbuf);

          if (cmdByte == MY_CMD) {
            mqttLog("activando bocina");
            //digitalWrite(BUZZER_PIN, LOW);
            //delay(1000);
            //digitalWrite(BUZZER_PIN, HIGH);
            mqttLog("enviando K");
            delay(500);
            sendAck();
          }
        }
        state = 0;
        break;
    }
  }
}

void mqttConnect() {
  while (!mqtt.connected()) {
    if (mqtt.connect("receiverNano32")) {
      mqtt.subscribe(TOPIC_CMD);
      mqttLog("MQTT conectado");
    } else {
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 4, 3);  // RX=D4, TX=D3
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);

  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.print(" IP: "); Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("pltrx-receiver");
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqttConnect();
  mqttLog("listo - 9600bps");
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // reservado para uso futuro
}

void loop() {
  ArduinoOTA.handle();

  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();

  handlePowerline();

  delay(10);
}
