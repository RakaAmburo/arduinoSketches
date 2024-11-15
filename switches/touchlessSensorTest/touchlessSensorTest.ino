// Define the pin where the touch sensor is connected
const int touchPin = 2;   // Digital pin for touch sensor
const int ledPin = 13;    // Built-in LED pin on Arduino (you can use any other pin)

// Variable to store the state of the touch sensor
int touchState = 0;

void setup() {
  // Initialize the touch pin as an input
  pinMode(touchPin, INPUT);

  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);

  // Start serial communication for debugging (optional)
  Serial.begin(9600);
}

void loop() {
  // Read the state of the touch sensor
  touchState = digitalRead(touchPin);

  // If the sensor is touched, turn on the LED
  if (touchState == HIGH) {
    digitalWrite(ledPin, HIGH);
    Serial.println("Touched! LED ON");
  } 
  // If the sensor is not touched, turn off the LED
  else {
    digitalWrite(ledPin, LOW);
    Serial.println("Not touched! LED OFF");
  }

  // Add a short delay to avoid bouncing effects
  delay(50);
}
