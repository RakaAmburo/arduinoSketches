// This is the RECEPTOR!!! with udp messaging for nodemcu
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Set WiFi credentials
#define WIFI_SSID "MIWIFI_mcCb"
#define WIFI_PASS "4ERT3RhP"
SoftwareSerial SUART(D3, D4);  //SRX = D3, STX = D4
const char* serverUrl = "http://192.168.1.135:8181/exec";

HTTPClient http;
WiFiClient wifiClient;

void setup() {
  Serial.begin(115200);
  SUART.begin(9600);

  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  Serial.println("CONNECTED");
}

void loop() {
  if (SUART.available()) {

    String inputString = SUART.readString();
    char message[inputString.length() + 1];
    inputString.toCharArray(message, inputString.length() + 1);
    sendExec(message);
  }
  delay(1000);
}

void sendExec(String message) {
  http.begin(wifiClient, serverUrl);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"possibleMessages\":[\"execute knock\"],\"extras\": {\"knocks\": \"" + message + "\"}}";
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