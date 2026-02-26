/*
====================================================
                FLEXÓMETRO
GPIO4  (SIN CAMBIOS)
====================================================
*/
const int flexPin = 4;
int flexValue;

/*
====================================================
                LIBRERÍAS
====================================================
*/
#include <Wire.h>
#include "MAX30105.h"

/*
====================================================
                I2C INDEPENDIENTES
====================================================
*/
TwoWire I2C_MAX = TwoWire(0);   // Para MAX30102
TwoWire I2C_MPU = TwoWire(1);   // Para MPU9250

/*
====================================================
                MAX30102
SDA 8
SCL 9
====================================================
*/
#define SDA_MAX 8
#define SCL_MAX 9

MAX30105 particleSensor;

long irValue;
long irFiltered = 0;
long previousIr = 0;

unsigned long lastBeat = 0;
float bpm = 0;
float bpmAvg = 0;

const byte RATE_SIZE = 8;
byte rates[RATE_SIZE];
byte rateSpot = 0;

/*
====================================================
                MPU9250
SDA 2
SCL 3
====================================================
*/
#define SDA_MPU 2
#define SCL_MPU 3
#define MPU_ADDR 0x68

#define ACCEL_SCALE 16384.0
#define GYRO_SCALE 131.0

#define GYRO_BIAS_X  -554.73
#define GYRO_BIAS_Y   483.33
#define GYRO_BIAS_Z     5.64

#define ACC_BIAS_X  (644.0)
#define ACC_BIAS_Y  (-760.0)
#define ACC_BIAS_Z  (18048.0 - 16384.0)

int16_t axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw;
float roll = 0, pitch = 0, yaw = 0;
unsigned long prevTime;

/* ================= MPU FUNC ================= */
void readMPU() {
  I2C_MPU.beginTransmission(MPU_ADDR);
  I2C_MPU.write(0x3B);
  I2C_MPU.endTransmission(false);
  I2C_MPU.requestFrom(MPU_ADDR, 14, true);

  axRaw = I2C_MPU.read()<<8|I2C_MPU.read();
  ayRaw = I2C_MPU.read()<<8|I2C_MPU.read();
  azRaw = I2C_MPU.read()<<8|I2C_MPU.read();
  I2C_MPU.read(); I2C_MPU.read();
  gxRaw = I2C_MPU.read()<<8|I2C_MPU.read();
  gyRaw = I2C_MPU.read()<<8|I2C_MPU.read();
  gzRaw = I2C_MPU.read()<<8|I2C_MPU.read();
}

/* ================= SETUP ================= */
void setup() {

  Serial.begin(115200);

  /* ================= MAX30102 ================= */
  I2C_MAX.begin(SDA_MAX, SCL_MAX);

  if (!particleSensor.begin(I2C_MAX, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 no encontrado");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x3F);

  /* ================= MPU9250 ================= */
  I2C_MPU.begin(SDA_MPU, SCL_MPU);

  I2C_MPU.beginTransmission(MPU_ADDR);
  I2C_MPU.write(0x6B);
  I2C_MPU.write(0x00);
  I2C_MPU.endTransmission();

  readMPU();

  float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
  float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
  float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;

  roll = atan2(ay, az) * 180 / PI;
  pitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;

  prevTime = micros();
}

/* ================= LOOP ================= */
void loop() {

  /* ================= FLEX ================= */
  flexValue = analogRead(flexPin);

  /* ================= MAX30102 ================= */
  irValue = particleSensor.getIR();

  if (irValue > 50000) {

    irFiltered = (previousIr * 0.9) + (irValue * 0.1);
    previousIr = irFiltered;

    if (irFiltered > 60000 && millis() - lastBeat > 300) {

      unsigned long delta = millis() - lastBeat;
      lastBeat = millis();

      bpm = 60.0 / (delta / 1000.0);

      if (bpm > 40 && bpm < 200) {
        rates[rateSpot++] = (byte)bpm;
        rateSpot %= RATE_SIZE;

        bpmAvg = 0;
        for (byte i = 0; i < RATE_SIZE; i++)
          bpmAvg += rates[i];
        bpmAvg /= RATE_SIZE;
      }
    }
  }

  /* ================= MPU9250 ================= */
  unsigned long now = micros();
  float dt = (now - prevTime) / 1000000.0;
  prevTime = now;

  readMPU();

  float gx = (gxRaw - GYRO_BIAS_X) / GYRO_SCALE;
  float gy = (gyRaw - GYRO_BIAS_Y) / GYRO_SCALE;
  float gz = (gzRaw - GYRO_BIAS_Z) / GYRO_SCALE;

  float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
  float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
  float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;

  float accRoll = atan2(ay, az) * 180 / PI;
  float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;

  roll = 0.96 * (roll + gx * dt) + 0.04 * accRoll;
  pitch = 0.96 * (pitch + gy * dt) + 0.04 * accPitch;
  yaw += gz * dt;

  /* ================= PAYLOAD ================= */
  String payload = "{";
  payload += "\"flex\":" + String(flexValue) + ",";
  payload += "\"bpm\":" + String(bpm,2) + ",";
  payload += "\"bpmAvg\":" + String(bpmAvg,2) + ",";
  payload += "\"roll\":" + String(roll,2) + ",";
  payload += "\"pitch\":" + String(pitch,2) + ",";
  payload += "\"yaw\":" + String(yaw,2);
  payload += "}";

  Serial.println(payload);

  delay(50);
}