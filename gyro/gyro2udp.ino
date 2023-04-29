#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

WiFiUDP Udp;

const char *ssid = "";
const char *password = "";

int contconexion = 0;

//Direccion I2C de la IMU
#define MPU 0x68

//Convertion ratios
#define A_R 16384.0  // 32768/2
#define G_R 131.0    // 32768/250

//rad to degrees convertion
#define RAD_A_DEG = 57.295779

//raw values
int16_t AcX, AcY, AcZ, GyX, GyY, GyZ;

//Angles
float Acc[2];
float Gy[3];
float Angle[3];

String values;

long tiempo_prev;
float dt;

void setup() {
  Wire.begin(D7, D6);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);  //soft ap normal
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED and contconexion < 50) {
    ++contconexion;
    delay(500);
    Serial.print(".");
  }
  if (contconexion < 50) {

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Conection error");
  }
}

void loop() {

  //Read mpu values
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  AcX = Wire.read() << 8 | Wire.read();  //each value ocupy 2
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  //accelerometer x,y tangent calculation
  Acc[1] = atan(-1 * (AcX / A_R) / sqrt(pow((AcY / A_R), 2) + pow((AcZ / A_R), 2))) * RAD_TO_DEG;
  Acc[0] = atan((AcY / A_R) / sqrt(pow((AcX / A_R), 2) + pow((AcZ / A_R), 2))) * RAD_TO_DEG;

  //gyro value reading
  Wire.beginTransmission(MPU);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();

  //angle calcutation
  Gy[0] = GyX / G_R;
  Gy[1] = GyY / G_R;
  Gy[2] = GyZ / G_R;

  dt = (millis() - tiempo_prev) / 1000.0;
  tiempo_prev = millis();

  Angle[0] = 0.98 * (Angle[0] + Gy[0] * dt) + 0.02 * Acc[0];
  Angle[1] = 0.98 * (Angle[1] + Gy[1] * dt) + 0.02 * Acc[1];

  Angle[2] = 0.98 * Angle[2] + Gy[2] * dt;

  values = String(Angle[0]) + "," + String(Angle[1]) + "," + String(Angle[2]);
  //Serial.println(valores);

  //UDP settings
  IPAddress ip = WiFi.localIP();
  ip[3] = 255;
  Udp.beginPacket(ip, 246);
  for (int i = 0; i < values.length(); i++) {
    Udp.write(byte(values[i]));
  }
  Udp.endPacket();

  delay(10);
}
