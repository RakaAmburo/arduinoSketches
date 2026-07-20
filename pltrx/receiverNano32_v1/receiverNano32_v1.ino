#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include "secrets.h"

#define BUZZER_PIN  2
#define MQTT_BROKER "192.168.1.135"
#define MQTT_PORT   1883
#define TOPIC_CMD   "pltrx/receiver/cmd"
#define TOPIC_LOG   "pltrx/receiver/log"

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

void sendB() {
  mqttLog("enviando B...");
  Serial1.write("B\n");
  Serial1.flush();
  delay(500);
  while (Serial1.available()) Serial1.read();
  waitAck();
}

void handlePowerline() {
  if (Serial1.available()) {
    delay(200);
    String str = "";
    while (Serial1.available()) {
      str += (char)Serial1.read();
    }
    str.trim();
    char logbuf[64];
    snprintf(logbuf, sizeof(logbuf), "recibido: [%s]", str.c_str());
    mqttLog(logbuf);

    if (str.indexOf("B") != -1) {
      mqttLog("B encontrado - activando bocina");
      EEPROM.begin(512);
      for (int i = 0; i < (int)str.length() && i < 252; i++)
        EEPROM.write(i, str[i]);
      EEPROM.write(str.length(), '\0');
      EEPROM.commit();

      digitalWrite(BUZZER_PIN, LOW);
      delay(1000);
      digitalWrite(BUZZER_PIN, HIGH);

      mqttLog("enviando K");
      delay(2000);
      Serial1.write("K\n");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[32] = {0};
  if (length >= sizeof(msg)) return;
  memcpy(msg, payload, length);
  if (strcmp(msg, "send B") == 0) sendB();
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

void loop() {
  ArduinoOTA.handle();

  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();

  handlePowerline();

  delay(10);
}