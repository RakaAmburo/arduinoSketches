//This is the EMITER!!! with movement sensor, nano or uno

#include<SoftwareSerial.h>
SoftwareSerial SUART(3, 4); //SRX = DPin-3, STX = DPin-4
int sensor = 2;

void setup()
{
  Serial.begin(9600);
  SUART.begin(9600);
  pinMode(sensor, INPUT); // declare sensor as input
}

void loop()
{
  long state = digitalRead(sensor);
  delay(1000);
  if (state == HIGH) {
    //digitalWrite (Status, HIGH);
    Serial.println("Motion detected!");
    SUART.print('A');
  }

  // SUART.print('A');
  // byte n = SUART.available();
  // if (n != 0)
  // {
  //   char x = SUART.read();
  //   Serial.println(x);
  // }
  // delay(2000);
}