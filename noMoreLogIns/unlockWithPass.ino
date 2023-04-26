#include "HID-Project.h"

int button = 3;
void setup() {
  BootKeyboard.begin();
  pinMode(button, INPUT_PULLUP);
  Serial.begin(9600);
}
void loop() {
  if (digitalRead(button) == LOW) {
    if (BootKeyboard.getLeds() & LED_CAPS_LOCK) {
      BootKeyboard.write(KEY_CAPS_LOCK);
    }
    Keyboard.press(KEY_UP_ARROW);
    delay(5);
    Keyboard.release(KEY_UP_ARROW);
    delay(1500);
    Keyboard.print("yourPassWord");
    delay(300);
    Keyboard.press(KEY_RETURN);
    Keyboard.release(KEY_RETURN);
  }
  delay(200);
}
