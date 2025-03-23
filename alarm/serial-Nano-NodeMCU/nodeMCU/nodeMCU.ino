// This is the RECEPTOR!!! with udp messaging for nodemcu
#include<SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Set WiFi credentials
#define WIFI_SSID ""
#define WIFI_PASS ""
#define UDP_PORT 8286
SoftwareSerial SUART(D3, D4); //SRX = D3, STX = D4

//UDP
WiFiUDP UDP;

void setup()
{
  Serial.begin(115200);
  SUART.begin(9600);

   WiFi.setOutputPower(5);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  // Begin listening to UDP port
  UDP.begin(UDP_PORT);  
}

void loop()
{
  //SUART.print('B');
  byte n = SUART.available();
  if (n != 0)
  {
    char x = SUART.read();
    Serial.println(x);
      //get the local ip network
      IPAddress ip = WiFi.localIP();
      ip[3] = 255;  // change last octect for bradcast
      UDP.beginPacket(ip, 8284);

      char message[] = "MUVEMENT_DETECTED";
      UDP.write(message);

      UDP.endPacket();
  }
  delay(1000);
}