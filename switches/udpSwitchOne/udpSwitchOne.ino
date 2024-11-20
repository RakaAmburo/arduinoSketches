#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Set WiFi credentials
#define WIFI_SSID ""
#define WIFI_PASS ""
#define UDP_PORT 8286

const int touchPin = D6;
const int ledPin = 2;
int touchStatus = HIGH;
int prevTouchStatus = HIGH;


// UDP
WiFiUDP UDP;
char packet[255];
char reply[] = "0";

bool s1 = false;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(touchPin, INPUT);
  pinMode(D1, OUTPUT);
  delay(100);
  digitalWrite(D1, LOW);
  digitalWrite(ledPin, HIGH);

  // Setup serial port
  Serial.begin(115200);

  WiFi.setPhyMode(WIFI_PHY_MODE_11G);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("CONNECTED");

  // Begin listening to UDP port
  UDP.begin(UDP_PORT);

  //Check the starting status of the button just in case
  touchStatus = digitalRead(touchPin);
  prevTouchStatus = digitalRead(touchPin);
}

void loop() {

  bool doSomeThing = false;
  //check and receive action signal from UDP
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    int len = UDP.read(packet, 255);
    if (len > 0) {
      packet[len] = '\0';
    }


    String cmd = charToString(packet);
    Serial.println(cmd);

    doSomeThing = (cmd.startsWith("BALCONY_")) ? true : false;

    if (cmd == "BALCONY_ON") {
      digitalWrite(D1, HIGH);
      digitalWrite(ledPin, LOW);
      s1 = true;
      Serial.println("on");
    } else if (cmd == "BALCONY_OFF") {
      digitalWrite(D1, LOW);
      digitalWrite(ledPin, HIGH);
      s1 = false;
      Serial.println("off");
    } else if (cmd == "BALCONY_STATUS") {
      Serial.println(digitalRead(touchPin));
      //DO NOTHING JUST SEND THE STATUS
    } else {
      doSomeThing = false;
    }

    if (doSomeThing) {
      delay(150);
      //get the local ip network
      IPAddress ip = WiFi.localIP();
      ip[3] = 255;  // change last octect for bradcast
      UDP.beginPacket(ip, 8284);

      String resutl = String(s1);

      reply[0] = (s1) ? '1' : '0';
      UDP.write(reply);

      UDP.endPacket();
    }
  }
  touchStatus = digitalRead(touchPin);
  if (touchStatus != prevTouchStatus) {
    prevTouchStatus = touchStatus;
    //d1Status = !d1Status;
    s1 = !s1;
    digitalWrite(ledPin, !s1);
    digitalWrite(D1, s1);
    Serial.println("change");
    Serial.println(s1);
  }

  delay(10);
}

String charToString(const char S[]) {
  byte at = 0;
  const char *p = S;
  String D = "";

  while (*p++) {
    D.concat(S[at++]);
  }

  return D;
}
