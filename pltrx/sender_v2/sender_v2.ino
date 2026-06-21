#include <SoftwareSerial.h>

SoftwareSerial mySerial(3, 4);  // RX, TX

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  pinMode(7, INPUT_PULLUP);
}

void waitAck() {
  unsigned long t = millis();
  while (millis() - t < 15000) {
    if (mySerial.available()) {
      byte b = mySerial.read();
      if (b == 'K') {
        Serial.println("recibido ok");
        return;
      }
    }
  }
  Serial.println("sin respuesta");
}

void sendB() {
  mySerial.write("B\n");
  Serial.println("enviando B...");
  delay(500);
  while (mySerial.available()) mySerial.read();  // limpia buffer de ruido
  waitAck();
  while (Serial.available()) Serial.read();      // limpia basura Serial tras waitAck
}

void loop() {
  if (Serial.available()) {
    char buffer[10];
    int i = 0;
    while (Serial.available() && i < 9) {
      char c = Serial.read();
      if (c == '\n') break;
      buffer[i++] = c;
    }
    buffer[i] = '\0';
    while (Serial.available()) Serial.read();    // limpia resto del buffer
    if (strcmp(buffer, "send B") == 0) sendB();
  }

  if (digitalRead(7) == LOW) {
    sendB();
    delay(500);
  }

  delay(12);
}