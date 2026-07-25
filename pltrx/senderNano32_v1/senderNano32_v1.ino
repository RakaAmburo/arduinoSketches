#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "secrets.h"

#define BUTTON_PIN  D7
#define MQTT_BROKER "192.168.1.135"
#define MQTT_PORT   1883
#define TOPIC_CMD   "pltrx/cmd"
#define TOPIC_LOG   "pltrx/log"

#define PL_PREAMBLE 0xAA
#define PL_SUFFIX   0xFF

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

void mqttLog(const char* msg) {
  Serial.println(msg);
  mqtt.publish(TOPIC_LOG, msg);
}

void waitAck() {
  unsigned long t = millis();
  while (millis() - t < 15000) {
    if (Serial1.available()) {
      byte b = Serial1.read();
      if (b == 'K') { mqttLog("ACK recibido ok"); return; }
    }
  }
  mqttLog("sin respuesta (timeout)");
}

void sendCmd(char cmd) {
  char logbuf[32];
  snprintf(logbuf, sizeof(logbuf), "enviando cmd: %c", cmd);
  mqttLog(logbuf);
  Serial1.write((byte)PL_PREAMBLE);
  Serial1.write((byte)cmd);
  Serial1.write((byte)PL_SUFFIX);
  Serial1.flush();
  delay(500);
  while (Serial1.available()) Serial1.read();  // limpia ruido
  waitAck();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (length == 0 || length > 1) return;
  sendCmd((char)payload[0]);
}

void mqttConnect() {
  while (!mqtt.connected()) {
    if (mqtt.connect("senderNano32")) {
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
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.print(" IP: "); Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("pltrx-sender");
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqttConnect();
  mqttLog("listo - 9600bps");
}

void loop() {
  ArduinoOTA.handle();

  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();

  if (digitalRead(BUTTON_PIN) == LOW) {
    sendCmd('B');
    delay(500);
  }

  delay(10);
}
