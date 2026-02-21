#include <BleCombo.h>
#include "esp_sleep.h"

const uint8_t JOYSTICK_X_PIN = 35;
const uint8_t JOYSTICK_Y_PIN = 34;
const uint8_t LEFT_BUTTON_PIN = 33;
const uint8_t RIGHT_BUTTON_PIN = 25;
const uint8_t PAGE_UP_BUTTON_PIN = 26;
const uint8_t PAGE_DOWN_BUTTON_PIN = 27;
const int JOYSTICK_THRESHOLD = 8;
const int BUTTONS_THRESHOLD = 300;
const int CONSECUTIVE_REQUIRED_POSITIVES = 6;
const unsigned long INACTIVITY_TIMEOUT_MS = 2 * 60 * 1000;

int16_t joystickXValue, joystickYValue;
int stableCountX = 0;
int stableCountY = 0;
unsigned long lastActivityTime;

void enterDeepSleep() {
  Serial.println("No se detecto actividad. Entrando en Deep Sleep...");
  Keyboard.end();
  Mouse.end();
  delay(100);
  
  const uint64_t wake_mask = (1ULL << PAGE_UP_BUTTON_PIN);
  esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    Serial.println("Despertado por un boton!");
    Keyboard.begin();
    Mouse.begin();
  } else {
    Keyboard.begin();
    Mouse.begin();
  }

  Serial.println("Staring mouse");
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PAGE_UP_BUTTON_PIN, INPUT);
  pinMode(PAGE_DOWN_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);

  lastActivityTime = millis();
}

bool isStableSignal(int8_t reading, int &stableCount) {
  if (abs(reading) > JOYSTICK_THRESHOLD) {
    if (stableCount < CONSECUTIVE_REQUIRED_POSITIVES) {
      stableCount++;
    }
  } else {
    stableCount = 0;
    return false;
  }
  return (stableCount == CONSECUTIVE_REQUIRED_POSITIVES);
}

void moveMouseGradually(int x, int y) {
  int xSpeed = map(abs(x), 0, 127, 1, 10);
  int ySpeed = map(abs(y), 0, 127, 1, 10);
  Mouse.move(x > 0 ? xSpeed : -xSpeed, y > 0 ? -ySpeed : ySpeed);
}

void loop() {
  // CORRECCIÓN 1: Verificar conexión BLE
  if (!Keyboard.isConnected()) {
    delay(100);
    return;
  }
  
  if (millis() - lastActivityTime >= INACTIVITY_TIMEOUT_MS) {
    enterDeepSleep();
  }

  joystickXValue = analogRead(JOYSTICK_X_PIN);
  joystickYValue = analogRead(JOYSTICK_Y_PIN);

  int8_t xMovement = map(joystickXValue, 0, 4095, -127, 127);
  int8_t yMovement = map(joystickYValue, 0, 4095, -127, 127);

  if (isStableSignal(xMovement, stableCountX) || isStableSignal(yMovement, stableCountY)) {
    lastActivityTime = millis();

    xMovement = abs(xMovement) > JOYSTICK_THRESHOLD ? xMovement : 0;
    yMovement = abs(yMovement) > JOYSTICK_THRESHOLD ? yMovement : 0;

    moveMouseGradually(xMovement, yMovement);
  }

  if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
    lastActivityTime = millis();
    Mouse.click(MOUSE_LEFT);
    Serial.println("Left Mouse button clicked!");
    delay(BUTTONS_THRESHOLD);
  }

  if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
    lastActivityTime = millis();
    Keyboard.press(KEY_LEFT_CTRL);
    Mouse.click(MOUSE_LEFT);
    delay(100);
    Keyboard.release(KEY_LEFT_CTRL);
    Serial.println("Ctrl + Right Mouse button clicked!");
    delay(BUTTONS_THRESHOLD);
  }

  if (digitalRead(PAGE_DOWN_BUTTON_PIN) == LOW) {
    lastActivityTime = millis();
    Keyboard.press(KEY_PAGE_DOWN);
    delay(100);
    Keyboard.release(KEY_PAGE_DOWN);
    Serial.println("Page down pressed and released!");
    delay(BUTTONS_THRESHOLD);
  }

  if (digitalRead(PAGE_UP_BUTTON_PIN) == LOW) {
    lastActivityTime = millis();
    Keyboard.press(KEY_PAGE_UP);
    delay(100);
    Keyboard.release(KEY_PAGE_UP);
    Serial.println("Page up pressed and released!");
    delay(BUTTONS_THRESHOLD);
  }

  // CORRECCIÓN 2: Aumentar delay de 12 a 30ms
  delay(30);
}