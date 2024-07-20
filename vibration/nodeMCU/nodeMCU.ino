// This is the RECEPTOR!!! with udp messaging for nodemcu
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Set WiFi credentials
#define WIFI_SSID "MIWIFI_mcCb"
#define WIFI_PASS "4ERT3RhP"
#define UDP_PORT 8286
SoftwareSerial SUART(D3, D4);  //SRX = D3, STX = D4

//UDP
WiFiUDP UDP;

void setup() {
  Serial.begin(115200);
  SUART.begin(9600);

  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  Serial.println("CONNECTED");
  // Begin listening to UDP port
  UDP.begin(UDP_PORT);
}

void loop() {
  if (SUART.available()) {

    String inputString = SUART.readString();
    //get the local ip network
    IPAddress ip = WiFi.localIP();
    ip[3] = 255;  // change last octect for bradcast
    UDP.beginPacket(ip, 8284);
    char message[inputString.length() + 1];
    inputString.toCharArray(message, inputString.length() + 1);
    UDP.write(message);
    UDP.endPacket();
  }
  delay(1000);
}