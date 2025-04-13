#include <WiFi.h>
#include <WiFiUdp.h>

#define UDP_PORT 8286
#define WIFI_SSID "MIWIFI_mcCb"
#define WIFI_PASS "4ERT3RhP"

const int touch1Pin = D6;
int touch1Status = HIGH;
int touch1PrevStatus = HIGH;
const int touch2Pin = D5;
int touch2Status = HIGH;
int touch2PrevStatus = HIGH;

const int initInterval = 5000;
const int deleyedInterval = 60000;
unsigned long lastCheck = 0;
unsigned long checkInterval = initInterval;
int connAttempts = 0;
const int maxFiledAttempts = 10;

// UDP
WiFiUDP UDP;
char packet[255];
char reply[] = "0:0";

bool s1 = false;
bool s2 = false;

void setup() {

  pinMode(touch1Pin, INPUT);
  pinMode(touch2Pin, INPUT);
  pinMode(D2, OUTPUT);
  delay(100);
  digitalWrite(D2, LOW);
  pinMode(D3, OUTPUT);
  delay(100);
  digitalWrite(D3, LOW);

  Serial.begin(115200);

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  wifiCheckAndConnect();

  UDP.begin(UDP_PORT);

  //Check the starting status of the buttons just in case
  touch1Status = digitalRead(touch1Pin);
  touch1PrevStatus = digitalRead(touch1Pin);
  touch2Status = digitalRead(touch2Pin);
  touch2PrevStatus = digitalRead(touch2Pin);
}


void loop() {

  if (millis() - lastCheck > checkInterval) {
    lastCheck = millis();
    wifiCheckAndConnect();
  }

  bool doSomeThing = false;
  //check and receive action signal from UDP
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    int len = UDP.read(packet, 255);
    if (len > 0) {
      packet[len] = '\0';
    }

    String cmd = charToString(packet);

    doSomeThing = (cmd.startsWith("LAUNDRY_")) ? true : false;

    if (cmd == "LAUNDRY_LIGHT_ON") {
      digitalWrite(D2, HIGH);
      s1 = true;
    } else if (cmd == "LAUNDRY_LIGHT_OFF") {
      digitalWrite(D2, LOW);
      s1 = false;
    } else if (cmd == "LAUNDRY_FAN_ON") {
      digitalWrite(D3, HIGH);
      s2 = true;
    } else if (cmd == "LAUNDRY_FAN_OFF") {
      digitalWrite(D3, LOW);
      s2 = false;
    } else if (cmd == "LAUNDRY_STATUS") {
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
      reply[2] = (s2) ? '1' : '0';

      UDP.write((uint8_t *)reply, strlen(reply));
      UDP.endPacket();
    }
  }
  touch1Status = digitalRead(touch1Pin);
  if (touch1Status != touch1PrevStatus) {
    touch1PrevStatus = touch1Status;
    s1 = !s1;
    digitalWrite(D2, s1);
    //Serial.println(s1);
  }
  touch2Status = digitalRead(touch2Pin);
  if (touch2Status != touch2PrevStatus) {
    touch2PrevStatus = touch2Status;
    s2 = !s2;
    digitalWrite(D3, s2);
    //Serial.println(s2);
  }
  delay(10);
}

String charToString(const char S[]) {
  byte at = 0;
  const char *p = S;
  String result = "";

  while (*p++) {
    result.concat(S[at++]);
  }

  return result;
}

void wifiCheckAndConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    connAttempts++;
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    //Serial.println("Connecting...");

    if (connAttempts > maxFiledAttempts){ 
      checkInterval = deleyedInterval;
    }
  } else {
    connAttempts = 0;
    checkInterval = initInterval;
    //Serial.println("Connected doing nothing");
  }
}