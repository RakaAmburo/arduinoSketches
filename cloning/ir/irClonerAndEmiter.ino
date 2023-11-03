#define irLedPin 3
#include <MemoryFree.h>

const int signalsSize = 600;
const int oneSignalSize = 5;
const char WAITING_4_SIGNAL[] = "WAITING_4_SIGNAL";

int signals[signalsSize];
int signalsPointer = 0;
char oneSignal[oneSignalSize];
int oneSignalPointer = 0;

void setup(void) {
  pinMode(irLedPin, OUTPUT);
  digitalWrite(irLedPin, LOW);  //Make sure LED starts "off"
  Serial.begin(9600);           //Initialize Serial port
}

void loop() {
  String command;
  while (Serial.available()) {
    command = Serial.readString();
    command.trim();
  }

  if (command == "1") {
    Serial.println(freeMemory());
    irClone();
  } else if (command == "2") {
    Serial.println(freeMemory());
    readSignalsFromSerial();
    sendSignal();
  }

  delay(20);
}

//Method in charge of sending the signal
void sendSignal() {
  for (int i = 0; i < signalsPointer; i++) {
    pulseIR(i, signals[i]);
  }
  delay(1000);
}

boolean pulseIR(int index, int n) {
  int microsecs = n;
  if (index % 2 == 0) {
    cli();  // this turns off any background interrupts
    while (microsecs > 0) {
      // 38 kHz is about 13 microseconds high and 13 microseconds low
      digitalWrite(irLedPin, HIGH);  // this takes about 3 microseconds to happen
      delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
      digitalWrite(irLedPin, LOW);   // this also takes about 3 microseconds
      delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working

      // so 26 microseconds altogether
      microsecs -= 26;
    }

    sei();  // this turns them back on
  } else {
    delayMicroseconds(microsecs);
  }
  return false;
}

const char SIGNAL_END = '&';
const char SIGNAL_SEPARATOR = ',';
const char STOP_ACTION[] = "STOP_ACTION";
const char READING_SERIAL[] = "READING_SERIAL";
const char EMPTY_SIGNAL[] = "EMPTY_SIGNAL";
const char SIGNALS_OVERFLOW[] = "SIGNALS_OVERFLOW";
const char ONE_SIGNAL_OVERFLOW[] = "ONE_SIGNAL_OVERFLOW";
void readSignalsFromSerial() {
  Serial.println(READING_SERIAL);
  signalsPointer = 0;
  memset(signals, '\0', signalsSize);
  while (true) {
    if (Serial.available() > 0) {
      char ch = Serial.read();
      if (ch == '|') {
        Serial.println(STOP_ACTION);
        break;
      } else if (isDigit(ch)) {
        if (oneSignalPointer < oneSignalSize)
          oneSignal[oneSignalPointer++] = ch;
        else {
          Serial.println(ONE_SIGNAL_OVERFLOW);
          break;
        }
      } else if (ch == SIGNAL_END || ch == SIGNAL_SEPARATOR) {
        if (oneSignalPointer == 0) {
          Serial.println(EMPTY_SIGNAL);
          break;
        }
        if (signalsPointer < signalsSize)
          signals[signalsPointer++] = atoi(oneSignal);
        else {
          Serial.println(SIGNALS_OVERFLOW);
          break;
        }
        memset(oneSignal, '\0', oneSignalSize);
        oneSignalPointer = 0;
        if (ch == SIGNAL_END) {
          break;
        }
      }
    }
  }
}

//empty the sensor memory
void irSensorFlush() {
  attachInterrupt(0, rxIR_Interrupt_Handler, CHANGE);
  delay(200);
  if (signalsPointer > 0) {
    delay(1000);
    detachInterrupt(0);
  }
}

void irClone() {
  irSensorFlush();
  memset(signals, '\0', signalsSize);
  signalsPointer = 0;
  attachInterrupt(0, rxIR_Interrupt_Handler, CHANGE);
  Serial.println(WAITING_4_SIGNAL);// Is ready when function is attached and sensor flushed
  delay(100);

  while (true) {
    if (stopAction()) {
      detachInterrupt(0);
      return;
    }
    if (signalsPointer > 0) {  //if a signal is captured
      delay(3000);//this could be potentialy less!
      detachInterrupt(0);                         //stop interrupts & capture until finshed here
      for (int i = 1; i < signalsPointer; i++) {  //now dump the times
        Serial.print(signals[i] - signals[i - 1]);
        if (i < signalsPointer - 1)
          Serial.print(F(","));
      }
      Serial.println("&");
      return;
    }
  }
}

void rxIR_Interrupt_Handler() {
  if (signalsPointer > signalsSize) return;
  signals[signalsPointer++] = micros();
}

boolean stopAction() {
  if (Serial.available() > 0) {
    char ch = Serial.read();
    if (ch == '|') {
      Serial.println(STOP_ACTION);
      return true;
    }
  }
  return false;
}
