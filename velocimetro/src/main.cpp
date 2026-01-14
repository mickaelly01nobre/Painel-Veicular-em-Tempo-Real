#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// ================= MPU6050 =================
float offsetX = 0, offsetY = 0;

#define FILTER_SIZE 10
float ax_buffer[FILTER_SIZE];
float ay_buffer[FILTER_SIZE];
int idx = 0;

#define LIMIAR_MOV 0.5
#define LIMIAR_CURVA 0.7
const float G = 9.81;

// ================= ULTRASSÔNICO =================
const int trigPin = 5;
const int echoPin = 18;

float medirDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long d = pulseIn(echoPin, HIGH, 30000);
  if (d == 0) return -1;
  return d * 0.000343 / 2.0;
}

void calibrarSensor() {
  const int N = 500;
  float sx = 0, sy = 0;
  for (int i = 0; i < N; i++) {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    sx += ax / 4096.0;
    sy += ay / 4096.0;
    delay(5);
  }
  offsetX = sx / N;
  offsetY = sy / N;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  mpu.initialize();
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_8);
  calibrarSensor();
}

void loop() {
  static unsigned long t0 = 0;
  if (millis() - t0 < 33) return; // ~30 Hz
  t0 = millis();

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  float ax_ms2 = ((ax / 4096.0) - offsetX) * G;
  float ay_ms2 = ((ay / 4096.0) - offsetY) * G;

  ax_buffer[idx] = ax_ms2;
  ay_buffer[idx] = ay_ms2;
  idx = (idx + 1) % FILTER_SIZE;

  float ax_f = 0, ay_f = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    ax_f += ax_buffer[i];
    ay_f += ay_buffer[i];
  }
  ax_f /= FILTER_SIZE;
  ay_f /= FILTER_SIZE;

  String estado = "Parado";
  if (ax_f > LIMIAR_MOV) estado = "Acelerando";
  else if (ax_f < -LIMIAR_MOV) estado = "Frenando";

  String curva = "Reta";
  if (ay_f > LIMIAR_CURVA) curva = "Direita";
  else if (ay_f < -LIMIAR_CURVA) curva = "Esquerda";

  float dist = medirDistancia();

  // CSV FINAL
  Serial.printf("%.2f,%s,%.2f,%s\n", ax_f, curva.c_str(), dist, estado.c_str());
}
