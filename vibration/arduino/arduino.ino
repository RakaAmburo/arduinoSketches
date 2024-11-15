#include <SoftwareSerial.h>
SoftwareSerial SUART(3, 4);

int nockStrength = 0;

const int nockMinStrength = 6;
const int falseNockTolerance = 110;  //dismiss readings near first detection
const int maxTimeBetweenNocks = 1500;
const int checkTimeInLoopNumber = 1000;

void setup() {
  Serial.begin(9600);
  SUART.begin(9600);
  pinMode(A0, INPUT);
}
long int activated = 0;
String message = "";
long int loops = 0;
void loop() {
  loops++;
  nockStrength = analogRead(A0);
  if (nockStrength > nockMinStrength) {
    long int current = millis();
    long int elapsed = current - activated;
    if (elapsed > falseNockTolerance) {  //-=++-
      activated = millis();
      if (elapsed <= maxTimeBetweenNocks) {
        if (message.length() > 0)
          message += ":";
        message += String(elapsed);
      } else {
        message = "";
      }
    }
  }
  if (loops > checkTimeInLoopNumber) {
    long int current = millis();
    long int elapsed = current - activated;
    // Starting over once max time reached
    if (elapsed > maxTimeBetweenNocks && message.length() > 0) {
      SUART.print(message);
      Serial.println(message);
      message = "";
    }
    loops = 0;
  }
}