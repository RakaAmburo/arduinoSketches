#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>

#define UDP_PORT 8286
#define WIFI_SSID "MIWIFI_mcCb"
#define WIFI_PASS "4ERT3RhP"
const char* serverUrl = "http://192.168.1.135:8181/alert";

unsigned long previousMillis = 0;
long interval = 5000;
const int sensorThreshold = 8;
const long normalInterval = 5000;
const long afterAlertInterval = 30000;

HTTPClient http;
WiFiClient wifiClient;

int pbIn = 5;
int count = 0;

void checkSensor() {
  if (count > sensorThreshold && interval != afterAlertInterval) {
    Serial.println(count);
    sendAlert();
    interval = afterAlertInterval;
  } else {
    Serial.print("not activated: ");
    Serial.println(count);
    interval = normalInterval;
  }
  count = 0;
}


void setup() {
  Serial.begin(9600);
  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("CONNECTED");
  attachInterrupt(pbIn, stateChange, FALLING);
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    checkSensor();
  }
}

IRAM_ATTR void stateChange()
{
  count++;
}

void sendAlert() {
  http.begin(wifiClient, serverUrl);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"possibleMessages\":[\"home bath movement detected\"]}";
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Resp: " + String(httpCode));
  } else {
    Serial.println("Error en la solicitud HTTP");
    Serial.println("Código de error: " + String(httpCode));
  }
  http.end();
}
