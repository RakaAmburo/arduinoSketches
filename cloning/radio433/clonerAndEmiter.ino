#define rfTransmitPin 4  //RF Transmitter pin = digital pin 4
#include <MemoryFree.h>

#define analogPin A0  // RF data pin = Analog pin 0


const int maxSignalLength = 255;
//const unsigned int upperThreshold = 300;
//const unsigned int lowerThreshold = 200;
const unsigned int upperThreshold = 250;
const unsigned int lowerThreshold = 250;
const int timeDelay = 105;  //slow down the signal transmission (can be from 75 - 135) 105
const int signalsSize = 600;
const int oneSignalSize = 5;

int signals[signalsSize];
int signalsPointer = 0;
char oneSignal[oneSignalSize];
int oneSignalPointer = 0;

int dataCounter = 0;

void setup() {
  Serial.begin(9600);
  pinMode(rfTransmitPin, OUTPUT);
}


void loop() {
  String command;
  while (Serial.available()) {
    command = Serial.readString();
    command.trim();
  }

  if (command == "1") {
    Serial.println(freeMemory());
    Serial.println("WAITING_4_SIGNAL");
    waitAndClone();
  } else if (command == "2") {
    Serial.println(freeMemory());
    readSignalsFromSerial();
    sendSignal();
  }

  delay(20);
}

void waitAndClone() {
  memset(signals, '\0', signalsSize);
  signalsPointer = 0;
  // wait until a LOW signal is received
  int analogResult = 0;
  while (analogResult < 80) {
    analogResult = analogRead(analogPin);
  }

  // got HIGH; read the rest of the data into dataBuffer
  for (int i = 0; i < signalsSize; i = i + 2) {

    dataCounter = 0;
    while (analogRead(analogPin) > upperThreshold && dataCounter < maxSignalLength)
      dataCounter++;
    signals[i] = dataCounter;

    // HIGH signal
    dataCounter = 0;
    while (analogRead(analogPin) < lowerThreshold && dataCounter < maxSignalLength)
      dataCounter++;
    signals[i + 1] = dataCounter;

    if (signals[i] == maxSignalLength && signals[i + 1] == 0
        || signals[i] == 0 && signals[i + 1] == maxSignalLength)
      break;

    signalsPointer = i;
  }

  //Serial.println("LOW,HIGH");
  delay(20);
  int size = signalsPointer;
  for (int i = 0; i < size; i = i + 2) {
    Serial.print(signals[i]);
    Serial.print(",");
    Serial.print(signals[i + 1]);
    if (i < size - 2)
      Serial.print(",");
    delay(20);
  }
  Serial.println("&");
}


//Method in charge of sending the signal
void sendSignal() {
  for (int i = 0; i <= 1; i++) {
    for (int i = 0; i < signalsPointer; i++) {
      sendRadioSignalSingleCommand(i, signals[i]);
    }
  }
  delay(1000);
}

boolean sendRadioSignalSingleCommand(int index, int n) {
  if (index % 2 == 0) {
    digitalWrite(rfTransmitPin, HIGH);
  } else {
    digitalWrite(rfTransmitPin, LOW);
  }
  delayMicroseconds(n * timeDelay);
  return false;
}

//reading from serial '\0' memset(integerArray, 0, sizeof(integerArray));
const char SIGNAL_END = '&';
const char SIGNAL_SEPARATOR = ',';
const char ARMED[] = "ARMED";
const char STOP_ACTION[] = "STOP_ACTION";
const char CLONING_RADIO[] = "CLONING_RADIO";
const char RADIO_SEND[] = "RADIO_SEND";
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
