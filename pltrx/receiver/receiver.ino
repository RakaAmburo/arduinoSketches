#include <SoftwareSerial.h>
#include <EEPROM.h>

const int buzzerPin = 2;

SoftwareSerial mySerial(3, 4);  // RX, TX
const int EEPROM_SIZE = 512;    // Tamaño total de la EEPROM
const int EEPROM_ADDRESS = 0;   // Dirección inicial en la EEPROM

void saveStringToEEPROM(int addr, String data) {
  int len = data.length();
  if (len > EEPROM_SIZE - addr - 1) {
    Serial.println("Error: String too long for EEPROM");
    return;
  }

  EEPROM.write(addr, len);  // Guarda la longitud del String

  for (int i = 0; i < len; i++) {
    EEPROM.write(addr + 1 + i, data[i]);  // Guarda cada carácter
  }
  EEPROM.write(addr + 1 + len, '\0');  // Agrega terminador nulo
}

String readStringFromEEPROM(int addr) {
  int len = EEPROM.read(addr);  // Lee la longitud del String
  char buffer[len + 1];

  for (int i = 0; i < len; i++) {
    buffer[i] = EEPROM.read(addr + 1 + i);  // Lee cada carácter
  }
  buffer[len] = '\0';  // Agrega terminador nulo

  return String(buffer);
}


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);


  //Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  //mySerial.println("Hello, world?");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
  //saveStringToEEPROM(EEPROM_ADDRESS, "setup");
}

void loop() {  // run over and over
  if (mySerial.available()) {
    String str = mySerial.readString();
    str.trim();
    if (str.indexOf("B") != -1) {
      Serial.println(str);
      saveStringToEEPROM(EEPROM_ADDRESS, str);
      mySerial.write("Ok");
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(buzzerPin, LOW);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(buzzerPin, HIGH);
    }
  }
  if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n');  // Lee hasta nueva línea
    inputString.trim();
    if (inputString.equals("show")) {
      String savedText = readStringFromEEPROM(EEPROM_ADDRESS);
      Serial.print("String leído desde la EEPROM: ");
      Serial.println(savedText);
    }
  }
  // if (Serial.available()) {
  //   mySerial.write(Serial.read());
  // }
}
