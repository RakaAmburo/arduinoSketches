#include <CommandParser.h>
#include "HID-Project.h"

typedef CommandParser<> MyCommandParser;

MyCommandParser parser;

void cmd_psw(MyCommandParser::Argument *args, char *response) {

  if (BootKeyboard.getLeds() & LED_CAPS_LOCK) {
    BootKeyboard.write(KEY_CAPS_LOCK);
  }
  String password = args[0].asString;
  password.trim();
  Keyboard.press(KEY_UP_ARROW);
  delay(5);
  Keyboard.release(KEY_UP_ARROW);
  delay(1500);
  Keyboard.print(password);
  delay(300);
  Keyboard.press(KEY_RETURN);
  Keyboard.release(KEY_RETURN);

  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_short_cut(MyCommandParser::Argument *args, char *response) {

  String dir = args[0].asString;
  if (dir.equals("right")) {
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(KEY_TAB);
    Keyboard.releaseAll();
  }

  if (dir.equals("left")) {
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(KEY_LEFT_SHIFT);
    Keyboard.press(KEY_TAB);
    Keyboard.releaseAll();
  }
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;
  BootKeyboard.begin();

  parser.registerCommand("PSW", "s", &cmd_psw);
  parser.registerCommand("SHORT", "s", &cmd_short_cut);
}

void loop() {
  if (Serial.available()) {
    char line[128];
    size_t lineLength = Serial.readBytesUntil('\n', line, 127);
    line[lineLength] = '\0';

    char response[MyCommandParser::MAX_RESPONSE_SIZE];
    parser.processCommand(line, response);
    Serial.println(response);
  }
}