#include "HID-Project.h"

void setup() {
  Serial.begin(9600);
  BootKeyboard.begin();
}

void loop() {
  if (Serial.available() > 0) {
    if (BootKeyboard.getLeds() & LED_CAPS_LOCK) {
      BootKeyboard.write(KEY_CAPS_LOCK);
    }
    String password = Serial.readString();
    password.trim();
    Keyboard.press(KEY_UP_ARROW);
    delay(5);
    Keyboard.release(KEY_UP_ARROW);
    delay(1500);
    Keyboard.print(password);
    delay(300);
    Keyboard.press(KEY_RETURN);
    Keyboard.release(KEY_RETURN);
  }
   delay(200);
}
