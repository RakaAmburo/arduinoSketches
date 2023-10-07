
#define IRledPin 3
#define NumIRsignals 71


int IRsignal[] = {
  // ON, OFF (in 10's of microseconds)
  // 1264, 412, 1264, 412, 428, 1252, 1264, 412, 1260, 416, 424, 1252, 424, 1252, 424, 1252, 428, 1252, 424, 1252, 1264, 412, 424, 7960, 1264, 412, 1264, 412, 428, 1252, 1260, 416, 1260, 412, 428, 1248, 428, 1252, 424, 1248, 428, 1256, 420, 1252, 1264, 412, 428, 9060, 1264, 412, 1264, 412, 424, 1252, 1264, 412, 1260, 412, 428, 1248, 428, 1252, 424, 1252, 424, 1252, 428, 1252, 1260, 416, 424, 7960, 1264, 412, 1264, 412, 424, 1252, 1264, 412, 1264, 412, 428, 1248, 424, 1256, 420, 1252, 424, 1252, 428, 1252, 1260, 412, 428, 9060, 1264, 412, 1264, 412, 424, 1252, 1264, 412, 1264, 412, 424, 1252, 424, 1252, 424, 1256, 420, 1256, 420, 1280, 1236, 412, 428, 7960, 1260, 416, 1260, 412, 428, 1252, 1260, 412, 1264, 412, 424, 1256, 424, 1252, 424, 1252, 424, 1248, 428, 1252, 1264, 412, 424, 0 
  //1264, 412, 1264, 412, 428, 1252, 1260, 412, 1264, 412, 1264, 412, 424, 1256, 424, 1252, 424, 1252, 424, 1256, 1260, 412, 1264, 7124, 1264, 412, 1264, 412, 428, 1252, 1260, 416, 1260, 416, 1260, 416, 424, 1252, 424, 1252, 424, 1252, 428, 1276, 1236, 416, 1264, 8224, 1264, 412, 1260, 416, 424, 1252, 1264, 412, 1264, 412, 1260, 416, 424, 1252, 424, 1256, 420, 1256, 424, 1252, 1260, 416, 1260, 7124, 1260, 416, 1264, 412, 424, 1252, 1264, 412, 1264, 412, 1260, 416, 424, 1252, 424, 1252, 424, 1252, 424, 1256, 1260, 416, 1260, 0
   1292, 384, 1264, 412, 424, 1252, 1264, 412, 1260, 416, 1260, 416, 424, 1252, 424, 1252, 424, 1252, 424, 1256, 1256, 416, 1260, 7124, 1260, 412, 1264, 416, 420, 1280, 1260, 388, 1260, 416, 1284, 388, 424, 1252, 424, 1256, 420, 1252, 424, 1280, 1236, 412, 1260, 8224, 1264, 412, 1260, 416, 424, 1252, 1260, 416, 1260, 412, 1260, 416, 424, 1256, 420, 1252, 424, 1252, 424, 1276, 1236, 416, 1284, 7100, 1260, 412, 1260, 416, 424, 1252, 1260, 416, 1260, 416, 1260, 412, 424, 1252, 424, 1248, 428, 1252, 424, 1276, 1240, 412, 1260, 8220, 1264, 412, 1264, 412, 424, 1256, 1256, 416, 1260, 416, 1260, 412, 424, 1256, 420, 1252, 448, 1228, 428, 1248, 1264, 412, 1260, 7124, 1260, 412, 1264, 412, 424, 1256, 1256, 416, 1260, 412, 1264, 412, 424, 1252, 424, 1256, 420, 1280, 396, 1256, 1260, 412, 1260, 8224, 1260, 412, 1264, 412, 448, 1232, 1260, 412, 1260, 412, 1264, 412, 424, 1256, 420, 1256, 420, 1256, 420, 1256, 1260, 412, 1260, 7124, 1260, 412, 1264, 412, 424, 1252, 1264, 412, 1260, 412, 1260, 416, 424, 1252, 424, 1252, 424, 1252, 424, 1252, 1260, 420, 1256, 0


};

String command;

void setup(void) {
  pinMode(IRledPin, OUTPUT);
  digitalWrite(IRledPin, LOW);  //Make sure LED starts "off"
  Serial.begin(9600);           //Initialize Serial port
}

void loop() {
  char data[6];
  int index = 0;

  delay(1000);  //Serial input seems to need some kind of short delay or the data gets screwed up.


  while (Serial.available()) {
    command = Serial.readString();
    command.trim();
  }


  if (command == "1") {  //If the Arduino receives the POWER signal...
    command = "";
    Serial.println("SENDING SIGNAL!");
    int arrSize = sizeof(IRsignal) / sizeof(int);
    Serial.println(arrSize);
    for (int x = 0; x <= 1; x++) {
      for (int i = 0; i < arrSize - 1; i += 2) {
        pulseIR(IRsignal[i]);
        delayMicroseconds(IRsignal[i + 1]);
      }
    }
  }
}


void pulseIR(long microsecs) {


  cli();

  while (microsecs > 0) {

    digitalWrite(IRledPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(IRledPin, LOW);
    delayMicroseconds(10);


    microsecs -= 26;
  }

  sei();
}
