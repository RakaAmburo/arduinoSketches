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

  if (Serial.available()) {
    char buffer[10];  // Buffer pequeño
    int i = 0;
    
    // Leer hasta salto de línea o máximo 9 caracteres
    while (Serial.available() && i < 9) {
      char c = Serial.read();
      if (c == '\n') break;
      buffer[i++] = c;
    }
    buffer[i] = '\0';  // Terminar string
    
    // Comparación directa (sin String)
    if (strcmp(buffer, "send") == 0) {
      mySerial.write("B");
      Serial.println("sending");
      delay(1000);
      
      // Respuesta rápida
      while (mySerial.available()) {
        Serial.write(mySerial.read());
      }
      Serial.println();  // Nueva línea después
    }
  }

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
