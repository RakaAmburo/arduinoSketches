#include <SoftwareSerial.h>
#include <EEPROM.h>

const int buzzerPin = 2;

SoftwareSerial mySerial(3, 4);  // RX, TX
const int EEPROM_SIZE = 512;
const int EEPROM_ADDRESS = 0;

void saveStringToEEPROM(int addr, String data) {
  int len = data.length();
  if (len > EEPROM_SIZE - addr - 1) return;
  EEPROM.write(addr, len);
  for (int i = 0; i < len; i++) EEPROM.write(addr + 1 + i, data[i]);
  EEPROM.write(addr + 1 + len, '\0');
}

String readStringFromEEPROM(int addr) {
  int len = EEPROM.read(addr);
  char buffer[len + 1];
  for (int i = 0; i < len; i++) buffer[i] = EEPROM.read(addr + 1 + i);
  buffer[len] = '\0';
  return String(buffer);
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  mySerial.setTimeout(2000);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
}

void loop() {
  if (mySerial.available()) {
    delay(200);
    String str = "";
    while (mySerial.available()) {
      str += (char)mySerial.read();
    }
    str.trim();
    Serial.print("recibido: [");
    Serial.print(str);
    Serial.println("]");

    if (str.indexOf("B") != -1) {
      Serial.println("B encontrado - activando bocina");
      saveStringToEEPROM(EEPROM_ADDRESS, str);
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(buzzerPin, LOW);
      delay(1000);
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(buzzerPin, HIGH);
      Serial.println("enviando K");
      delay(2000);
      mySerial.write("K\n");
    }
  }

  if (Serial.available() > 0) {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim();
    if (inputString.equals("show")) {
      String savedText = readStringFromEEPROM(EEPROM_ADDRESS);
      Serial.print("String leido desde la EEPROM: ");
      Serial.println(savedText);
    }
  }
}
