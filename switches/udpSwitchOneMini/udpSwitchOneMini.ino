#include <WiFi.h>
#include <WiFiUDP.h>

const char* ssid = "MIWIFI_mcCb";
const char* password = "4ERT3RhP";
#define UDP_PORT 8286;

const int touchPin = 4;
const int switchOne = 5;
const int ledPin = 3;
int touchStatus = HIGH;
int prevTouchStatus = HIGH;

// UDP
WiFiUDP UDP;
char packet[255];
char reply = '0';

bool s1 = false;

void setup() {

  pinMode(ledPin, OUTPUT);
  pinMode(touchPin, INPUT);
  pinMode(switchOne, OUTPUT);
  digitalWrite(switchOne, LOW);
  digitalWrite(ledPin, LOW);

  Serial.begin(115200);  // Inicia el monitor serial para ver la salida
  delay(1000);

  Serial.println("Conectando a Wi-Fi...");

  WiFi.begin(ssid, password);  // Inicia la conexión Wi-Fi

  // Espera hasta que se establezca la conexión
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("CONNECTED");

  // Begin listening to UDP port
  UDP.begin(8286);

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
      digitalWrite(switchOne, HIGH);
      digitalWrite(ledPin, LOW);
      s1 = true;
      Serial.println("on");
    } else if (cmd == "BALCONY_OFF") {
      digitalWrite(switchOne, LOW);
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

      reply = (s1) ? '1' : '0';
      UDP.write((uint8_t*)&reply, 1);

      UDP.endPacket();
    }
  }
  touchStatus = digitalRead(touchPin);
  if (touchStatus != prevTouchStatus) {
    prevTouchStatus = touchStatus;
    s1 = !s1;
    digitalWrite(ledPin, !s1);
    digitalWrite(switchOne, s1);
    Serial.println("change");
    Serial.println(s1);
  }

  delay(10);
}

String charToString(const char S[]) {
  byte at = 0;
  const char* p = S;
  String D = "";

  while (*p++) {
    D.concat(S[at++]);
  }

  return D;
}