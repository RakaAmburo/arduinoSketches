// Board: esp32:esp32:esp32 (ESP32-WROOM-32) | PartitionScheme: min_spiffs | Last upload: 2026-06-06
// bleMouse4 — version con WiFi + OTA + logs de debug
#include <BleCombo.h>
#include "esp_sleep.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "secrets.h"

const uint8_t JOYSTICK_X_PIN = 35;
const uint8_t JOYSTICK_Y_PIN = 34;
const uint8_t LEFT_BUTTON_PIN = 33;
const uint8_t RIGHT_BUTTON_PIN = 25;
const uint8_t PAGE_UP_BUTTON_PIN = 26;
const uint8_t PAGE_DOWN_BUTTON_PIN = 27;
const int JOYSTICK_THRESHOLD = 15;
const int BUTTONS_THRESHOLD = 300;
const int CONSECUTIVE_REQUIRED_POSITIVES = 8;
const unsigned long INACTIVITY_TIMEOUT_MS = 2 * 60 * 1000;
const int SCROLL_TICKS = 3;

int16_t joystickXValue, joystickYValue;
int stableCountX = 0;
int stableCountY = 0;
unsigned long lastActivityTime;
bool wifiEnabled = false;

void startWifi() {
  Serial.println("[WiFi] Iniciando conexion...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Conectado. IP: ");
    Serial.println(WiFi.localIP());
    ArduinoOTA.setHostname("air-mouse");
    ArduinoOTA.setPassword(OTA_PASS);
    ArduinoOTA.onStart([]() { Serial.println("[OTA] Inicio de actualizacion"); });
    ArduinoOTA.onEnd([]() { Serial.println("[OTA] Fin de actualizacion"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("[OTA] Progreso: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("[OTA] Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("[OTA] Listo. Esperando actualizaciones...");
  } else {
    Serial.println("[WiFi] ERROR: No se pudo conectar");
  }
}

void stopWifi() {
  ArduinoOTA.end();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("[WiFi] Apagado.");
}

void enterDeepSleep() {
  Serial.println("[Sleep] Sin actividad. Entrando en Deep Sleep...");
  Keyboard.end();
  Mouse.end();
  delay(100);
  const uint64_t wake_mask = (1ULL << PAGE_UP_BUTTON_PIN);
  esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  Serial.println("[Setup] Iniciando bleMouse4...");
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    Serial.println("[Setup] Despertado por boton.");
  }
  WiFi.mode(WIFI_OFF);
  Serial.println("[Setup] Iniciando BLE...");
  Keyboard.begin();
  Mouse.begin();
  Serial.println("[Setup] BLE listo.");
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PAGE_UP_BUTTON_PIN, INPUT);
  pinMode(PAGE_DOWN_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  lastActivityTime = millis();
  Serial.println("[Setup] Listo.");
}

bool isStableSignal(int8_t reading, int &stableCount) {
  if (abs(reading) > JOYSTICK_THRESHOLD) {
    if (stableCount < CONSECUTIVE_REQUIRED_POSITIVES) stableCount++;
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
  if (wifiEnabled) ArduinoOTA.handle();

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

  bool leftPressed = digitalRead(LEFT_BUTTON_PIN) == LOW;
  bool rightPressed = digitalRead(RIGHT_BUTTON_PIN) == LOW;

  if (leftPressed && rightPressed) {
    lastActivityTime = millis();
    wifiEnabled = !wifiEnabled;
    Serial.printf("[WiFi] Toggle: %s\n", wifiEnabled ? "ON" : "OFF");
    if (wifiEnabled) startWifi(); else stopWifi();
    delay(1000);
  } else {
    if (leftPressed) {
      lastActivityTime = millis();
      Mouse.click(MOUSE_LEFT);
      Serial.println("[Button] Left click");
      delay(BUTTONS_THRESHOLD);
    }
    if (rightPressed) {
      lastActivityTime = millis();
      Keyboard.press(KEY_LEFT_CTRL);
      Mouse.click(MOUSE_LEFT);
      delay(100);
      Keyboard.release(KEY_LEFT_CTRL);
      Serial.println("[Button] Ctrl+Left click");
      delay(BUTTONS_THRESHOLD);
    }
  }

  if (digitalRead(PAGE_UP_BUTTON_PIN) == LOW) {
    lastActivityTime = millis();
    Mouse.move(0, 0, SCROLL_TICKS);
    Serial.println("[Button] Scroll up");
    delay(BUTTONS_THRESHOLD);
  }

  if (digitalRead(PAGE_DOWN_BUTTON_PIN) == LOW) {
    lastActivityTime = millis();
    Mouse.move(0, 0, -SCROLL_TICKS);
    Serial.println("[Button] Scroll down");
    delay(BUTTONS_THRESHOLD);
  }

  delay(30);
}