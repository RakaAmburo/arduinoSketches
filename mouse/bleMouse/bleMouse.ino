#include <BleCombo.h>

const uint8_t JOYSTICK_X_PIN = 35;
const uint8_t JOYSTICK_Y_PIN = 34;
const uint8_t LEFT_BUTTON_PIN = 33;
const uint8_t RIGHT_BUTTON_PIN = 25;
const uint8_t PAGE_UP_BUTTON_PIN = 26;
const uint8_t PAGE_DOWN_BUTTON_PIN = 27;
const int JOYSTICK_THRESHOLD = 10;  // Threshold to ignore small movements near the center
const int BUTTONS_THRESHOLD = 100;   // Threshold to double click in millis

int16_t joystickXValue, joystickYValue;

void setup() {
  Serial.begin(115200);
  Serial.println("Staring mouse");
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PAGE_UP_BUTTON_PIN, INPUT);
  pinMode(PAGE_DOWN_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);

  Keyboard.begin();
  Mouse.begin();
  
}

void loop() {
  if (Keyboard.isConnected()) {
    // Read joystick values
    joystickXValue = analogRead(JOYSTICK_X_PIN);
    joystickYValue = analogRead(JOYSTICK_Y_PIN);

    // Mapping values to reduce readings to 127
    int8_t xMovement = map(joystickXValue, 0, 4095, -127, 127);
    int8_t yMovement = map(joystickYValue, 0, 4095, -127, 127);

    Serial.println("X mapped value: " + (String)xMovement);
    Serial.println("Y mapped value: " + (String)yMovement);
    Serial.println();

    // Remove noice from movement
    if (abs(xMovement) > JOYSTICK_THRESHOLD || abs(yMovement) > JOYSTICK_THRESHOLD) {

      xMovement = abs(xMovement) > JOYSTICK_THRESHOLD ? xMovement : 0;
      yMovement = abs(yMovement) > JOYSTICK_THRESHOLD ? yMovement : 0;

      moveMouseGradually(xMovement, yMovement);
    }

    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      Mouse.click(MOUSE_LEFT);
      Serial.println("Left Mouse button clicked!");
      delay(BUTTONS_THRESHOLD);
    }

    if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      Keyboard.press(KEY_LEFT_CTRL);
      Mouse.click(MOUSE_LEFT);
      delay(100);
      Keyboard.release(KEY_LEFT_CTRL);
      Serial.println("Ctrl + qRight Mouse button clicked!");
      delay(BUTTONS_THRESHOLD);
    }

    if (digitalRead(PAGE_DOWN_BUTTON_PIN) == LOW) {
      Keyboard.press(KEY_PAGE_DOWN);
      delay(100);
      Keyboard.release(KEY_PAGE_DOWN);
      Serial.println("Page down pressed and released!");
      delay(BUTTONS_THRESHOLD);
    }

    if (digitalRead(PAGE_UP_BUTTON_PIN) == LOW) {
      Keyboard.press(KEY_PAGE_UP);
      delay(100);
      Keyboard.release(KEY_PAGE_UP);
      Serial.println("Page up pressed and released!");
      delay(BUTTONS_THRESHOLD);
    }
  }
  delay(12);
}

int getSpeed(int value) {
  // Mapping to mouse sepped
  return map(abs(value), 0, 127, 1, 10);  // Speed from 1 to 10
}

void moveMouseGradually(int x, int y) {
  int xSpeed = getSpeed(x);
  int ySpeed = getSpeed(y);

  Mouse.move(x > 0 ? xSpeed : -xSpeed, y > 0 ? -ySpeed : ySpeed);
}