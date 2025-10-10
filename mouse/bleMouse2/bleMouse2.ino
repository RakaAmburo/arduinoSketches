#include <BleCombo.h>
#include "esp_sleep.h"  // Se agrega la libreria para Deep Sleep

const uint8_t JOYSTICK_X_PIN = 35;
const uint8_t JOYSTICK_Y_PIN = 34;
const uint8_t LEFT_BUTTON_PIN = 33;
const uint8_t RIGHT_BUTTON_PIN = 25;
const uint8_t PAGE_UP_BUTTON_PIN = 26;
const uint8_t PAGE_DOWN_BUTTON_PIN = 27;
const int JOYSTICK_THRESHOLD = 8;
const int BUTTONS_THRESHOLD = 300;
const int CONSECUTIVE_REQUIRED_POSITIVES = 6;
const unsigned long INACTIVITY_TIMEOUT_MS = 2 * 60 * 1000;  // 3 minutos en milisegundos

int16_t joystickXValue, joystickYValue;
int stableCountX = 0;
int stableCountY = 0;
unsigned long lastActivityTime;  // Se agrega la variable para el temporizador

// Funcion para que el ESP32 entre en Deep Sleep
void enterDeepSleep() {
  Serial.println("No se detecto actividad. Entrando en Deep Sleep...");

  // Apagamos los modulos Bluetooth para un maximo ahorro de energia
  Keyboard.end();
  Mouse.end();

  // Configuramos la interrupcion externa para despertar
  // Se usa 'esp_sleep_enable_ext1_wakeup' porque permite usar multiples pines
  // 'ESP_EXT1_WAKEUP_ANY_LOW' se activa con cualquier pin que cambie a LOW (presionando el boton)
  const uint64_t wake_mask = (1ULL << PAGE_UP_BUTTON_PIN) ;
  esp_sleep_enable_ext1_wakeup(wake_mask, ESP_EXT1_WAKEUP_ALL_LOW);

  // Entra en modo de bajo consumo
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  // Se verifica si la causa del reinicio es el despertar de Deep Sleep
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  // Si fue un reinicio por boton, se inicializa el bluetooth y la conexion
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

  // Se inicializa el tiempo de la ultima actividad en el momento de encendido
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
  // Verificamos si ha pasado el tiempo de inactividad
  if (millis() - lastActivityTime >= INACTIVITY_TIMEOUT_MS) {
    enterDeepSleep();
  }

  // Leemos los valores del joystick
  joystickXValue = analogRead(JOYSTICK_X_PIN);
  joystickYValue = analogRead(JOYSTICK_Y_PIN);

  // Mapeamos los valores para reducir las lecturas a 127
  int8_t xMovement = map(joystickXValue, 0, 4095, -127, 127);
  int8_t yMovement = map(joystickYValue, 0, 4095, -127, 127);

  // Verificamos si hay movimiento significativo del joystick
  if (isStableSignal(xMovement, stableCountX) || isStableSignal(yMovement, stableCountY)) {
    // Se detecta actividad, actualizamos el temporizador de inactividad
    //lastActivityTime = millis();

    xMovement = abs(xMovement) > JOYSTICK_THRESHOLD ? xMovement : 0;
    yMovement = abs(yMovement) > JOYSTICK_THRESHOLD ? yMovement : 0;

    moveMouseGradually(xMovement, yMovement);
  }

  // Leemos los botones y actualizamos el temporizador de actividad si se presionan
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
    Serial.println("Ctrl + qRight Mouse button clicked!");
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

  delay(12);
}