#include <SoftwareSerial.h>

SoftwareSerial mySerial(3, 4);  // RX, TX
long count = 0;
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);

  pinMode(7, INPUT);
}

void loop() {  // run over and over


  if (digitalRead(7) == LOW) {

    mySerial.write("B");
    Serial.println("sending");
    delay(1000);
    if (mySerial.available()) {
      Serial.println(mySerial.readString());
    }
    delay(200);
  }




  delay(12);
}
