#define LEDPIN 13

#define maxLen 500

volatile  unsigned int irBuffer[maxLen]; 
volatile unsigned int x = 0;

void setup() {
  Serial.begin(9600); 
  //by default reads pin 2
  attachInterrupt(0, rxIR_Interrupt_Handler, CHANGE);
}

void loop() {
  Serial.println(F("Press the button on the remote now - once only"));
  delay(5000); 
  if (x) { 
    digitalWrite(LEDPIN, HIGH);
    Serial.println();
    Serial.print(F("Raw: (")); 
    Serial.print((x - 1));
    Serial.print(F(") "));
    detachInterrupt(0);
    for (int i = 1; i < x; i++) { 
      
      Serial.print(irBuffer[i] - irBuffer[i - 1]);
      Serial.print(F(", "));
    }
    x = 0;
    Serial.println();
    Serial.println();
    digitalWrite(LEDPIN, LOW);
    attachInterrupt(0, rxIR_Interrupt_Handler, CHANGE);
  }

}

void rxIR_Interrupt_Handler() {
  if (x > maxLen) return; 
  irBuffer[x++] = micros();

}
