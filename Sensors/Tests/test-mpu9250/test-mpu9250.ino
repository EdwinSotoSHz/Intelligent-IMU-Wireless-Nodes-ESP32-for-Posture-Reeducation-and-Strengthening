#include <Wire.h>

#define SDA_PIN 33
#define SCL_PIN 32
#define MPU9250_ADDR 0x68

// Sensibilidades
#define ACCEL_SCALE 16384.0     // ±2g
#define GYRO_SCALE 131.0        // ±250°/s

int16_t ax, ay, az;
int16_t gx, gy, gz;

float ax_g, ay_g, az_g;
float gx_dps, gy_dps, gz_dps;

float gyroBiasX = 0, gyroBiasY = 0, gyroBiasZ = 0;
float accelBiasX = 0, accelBiasY = 0, accelBiasZ = 0;

float roll = 0;
float pitch = 0;

unsigned long lastTime;
float dt;

void writeRegister(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

void readMPU() {
  Wire.beginTransmission(MPU9250_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU9250_ADDR, 14, true);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read();
  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();
}

void calibrate() {
  Serial.println("No mover el sensor. Calibrando...");
  delay(2000);

  long sumGX = 0, sumGY = 0, sumGZ = 0;
  long sumAX = 0, sumAY = 0, sumAZ = 0;

  for (int i = 0; i < 2000; i++) {
    readMPU();
    sumGX += gx;
    sumGY += gy;
    sumGZ += gz;

    sumAX += ax;
    sumAY += ay;
    sumAZ += az;

    delay(2);
  }

  gyroBiasX = sumGX / 2000.0;
  gyroBiasY = sumGY / 2000.0;
  gyroBiasZ = sumGZ / 2000.0;

  accelBiasX = sumAX / 2000.0;
  accelBiasY = sumAY / 2000.0;
  accelBiasZ = (sumAZ / 2000.0) - ACCEL_SCALE; // quitar 1g

  Serial.println("Calibración lista");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  writeRegister(0x6B, 0x00); // Wake up
  writeRegister(0x1C, 0x00); // ±2g
  writeRegister(0x1B, 0x00); // ±250°/s

  delay(100);
  calibrate();

  lastTime = millis();
}

void loop() {
  readMPU();

  unsigned long currentTime = millis();
  dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;

  // Quitar bias
  gx_dps = (gx - gyroBiasX) / GYRO_SCALE;
  gy_dps = (gy - gyroBiasY) / GYRO_SCALE;
  gz_dps = (gz - gyroBiasZ) / GYRO_SCALE;

  ax_g = (ax - accelBiasX) / ACCEL_SCALE;
  ay_g = (ay - accelBiasY) / ACCEL_SCALE;
  az_g = (az - accelBiasZ) / ACCEL_SCALE;

  // Ángulos por acelerómetro
  float rollAcc = atan2(ay_g, az_g) * 180 / PI;
  float pitchAcc = atan2(-ax_g, sqrt(ay_g * ay_g + az_g * az_g)) * 180 / PI;

  // Filtro complementario
  roll = 0.98 * (roll + gx_dps * dt) + 0.02 * rollAcc;
  pitch = 0.98 * (pitch + gy_dps * dt) + 0.02 * pitchAcc;

  Serial.print("Roll: ");
  Serial.print(roll);
  Serial.print(" | Pitch: ");
  Serial.println(pitch);

  delay(10);
}