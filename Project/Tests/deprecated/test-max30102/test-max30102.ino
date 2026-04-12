#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define I2C_SDA 4
#define I2C_SCL 8
#define IR_THRESHOLD 50000
#define SMOOTH_SIZE 6
#define MIN_VALID_SAMPLES 3

uint32_t irBuffer[100];
uint32_t redBuffer[100];
int32_t bufferLength = 100;

int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

int32_t hrHistory[SMOOTH_SIZE]   = {0};
int32_t spo2History[SMOOTH_SIZE] = {0};
int histIndex = 0;
int validHRCount  = 0;
int validSPO2Count = 0;

int32_t smoothedValue(int32_t* history, int* validCount) {
  int32_t sum = 0;
  *validCount = 0;

  for (int i = 0; i < SMOOTH_SIZE; i++) {
    if (history[i] > 0) {
      sum += history[i];
      (*validCount)++;
    }
  }

  return (*validCount) > 0 ? sum / (*validCount) : 0;
}

bool fingerPresent() {
  uint32_t avg = 0;

  for (int i = 95; i < 100; i++) {
    avg += irBuffer[i];
  }

  return (avg / 5) > IR_THRESHOLD;
}

void collectSamples(byte start, byte end) {
  for (byte i = start; i < end; i++) {

    while (particleSensor.available() == false) {
      particleSensor.check();
    }

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i]  = particleSensor.getIR();

    particleSensor.nextSample();
  }
}

void resetHistories() {

  memset(hrHistory, 0, sizeof(hrHistory));
  memset(spo2History, 0, sizeof(spo2History));

  histIndex = 0;
  validHRCount = 0;
  validSPO2Count = 0;
}

void setup() {

  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 no detectado. Revisa pines 4 y 8.");
    while (1);
  }

  particleSensor.setup(50, 4, 2, 100, 411, 4096);

  Serial.println("Listo. Coloca el dedo con presión constante.");
}

void loop() {

  Serial.println("Esperando dedo...");

  while (true) {

    while (particleSensor.available() == false) {
      particleSensor.check();
    }

    irBuffer[99] = particleSensor.getIR();

    particleSensor.nextSample();

    if (irBuffer[99] > IR_THRESHOLD) {
      break;
    }

    delay(100);
  }

  resetHistories();

  collectSamples(0, 100);

  maxim_heart_rate_and_oxygen_saturation(
      irBuffer,
      bufferLength,
      redBuffer,
      &spo2,
      &validSPO2,
      &heartRate,
      &validHeartRate);

  while (true) {

    for (byte i = 25; i < 100; i++) {

      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    collectSamples(75, 100);

    if (!fingerPresent()) {
      Serial.println("Dedo retirado. Reiniciando...");
      break;
    }

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer,
        bufferLength,
        redBuffer,
        &spo2,
        &validSPO2,
        &heartRate,
        &validHeartRate);

    bool hrOk   = validHeartRate && heartRate >= 45 && heartRate <= 149;
    bool spo2Ok = validSPO2 && spo2 >= 85 && spo2 <= 100;

    if (hrOk) {
      hrHistory[histIndex] = heartRate;
    } else {
      hrHistory[histIndex] = 0;
    }

    if (spo2Ok) {
      spo2History[histIndex] = spo2;
    } else {
      spo2History[histIndex] = 0;
    }

    histIndex = (histIndex + 1) % SMOOTH_SIZE;

    int32_t displayHR   = smoothedValue(hrHistory, &validHRCount);
    int32_t displaySPO2 = smoothedValue(spo2History, &validSPO2Count);

    bool stable = (validHRCount >= MIN_VALID_SAMPLES) &&
                  (validSPO2Count >= MIN_VALID_SAMPLES);

    Serial.print("HR: ");

    if (displayHR > 0)
      Serial.print(String(displayHR) + " bpm");
    else
      Serial.print("--");

    Serial.print(" | SpO2: ");

    if (displaySPO2 > 0)
      Serial.print(String(displaySPO2) + "%");
    else
      Serial.print("--");

    Serial.print(" | Estado: ");

    if (stable)
      Serial.println("Estable");
    else
      Serial.println("Calculando...");
  }
}