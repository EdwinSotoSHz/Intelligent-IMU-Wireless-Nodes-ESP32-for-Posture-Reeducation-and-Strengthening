/*Inicia en 000*/
#include <Wire.h>
#include <math.h>

#define SDA_PIN 1
#define SCL_PIN 0
#define MPU_ADDR 0x68

#define ACCEL_SCALE 16384.0
#define GYRO_SCALE 131.0

#define GYRO_BIAS_X  -586.58
#define GYRO_BIAS_Y   503.27
#define GYRO_BIAS_Z    67.01

#define ACC_BIAS_X  1477.39
#define ACC_BIAS_Y   379.93
#define ACC_BIAS_Z  1670.55

int16_t axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw;
float roll=0, pitch=0, yaw=0;
float rollOffset=0, pitchOffset=0, yawOffset=0;
bool calibrated=false;
unsigned long prevTime, calibrationStart;
float rollSum=0, pitchSum=0, yawSum=0;
int calibrationSamples=0;

// ===== Lectura MPU =====
void readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  axRaw = Wire.read()<<8 | Wire.read();
  ayRaw = Wire.read()<<8 | Wire.read();
  azRaw = Wire.read()<<8 | Wire.read();
  Wire.read(); Wire.read();
  gxRaw = Wire.read()<<8 | Wire.read();
  gyRaw = Wire.read()<<8 | Wire.read();
  gzRaw = Wire.read()<<8 | Wire.read();
}

// ===== Normalización de ángulo [-180,180] =====
float normalizeAngle(float angle){
  while(angle > 180) angle -= 360;
  while(angle < -180) angle += 360;
  return angle;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(SDA_PIN, SCL_PIN);

  // Inicializar MPU
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);
  Wire.endTransmission();

  readMPU();
  float ax=(axRaw-ACC_BIAS_X)/ACCEL_SCALE;
  float ay=(ayRaw-ACC_BIAS_Y)/ACCEL_SCALE;
  float az=(azRaw-ACC_BIAS_Z)/ACCEL_SCALE;

  roll = atan2(ay, az)*180.0/PI;
  pitch = atan2(-ax, sqrt(ay*ay+az*az))*180.0/PI;

  prevTime = micros();
  calibrationStart = millis();

  Serial.println("MPU6050 inicializado");
  Serial.println("Calibrando 3 segundos...");
  Serial.println("Roll\tPitch\tYaw");
}

void loop() {
  unsigned long now = micros();
  float dt = (now-prevTime)/1000000.0;
  prevTime = now;

  readMPU();

  float gx=(gxRaw-GYRO_BIAS_X)/GYRO_SCALE;
  float gy=(gyRaw-GYRO_BIAS_Y)/GYRO_SCALE;
  float gz=(gzRaw-GYRO_BIAS_Z)/GYRO_SCALE;

  float ax=(axRaw-ACC_BIAS_X)/ACCEL_SCALE;
  float ay=(ayRaw-ACC_BIAS_Y)/ACCEL_SCALE;
  float az=(azRaw-ACC_BIAS_Z)/ACCEL_SCALE;

  float accRoll = atan2(ay, az)*180.0/PI;
  float accPitch = atan2(-ax, sqrt(ay*ay+az*az))*180.0/PI;

  // Filtro complementario con normalización
  roll = normalizeAngle(0.96*(roll+gx*dt)+0.04*accRoll);
  pitch = normalizeAngle(0.96*(pitch+gy*dt)+0.04*accPitch);

  // Yaw integrado pero limitado
  yaw += gz*dt;
  yaw = normalizeAngle(yaw);

  // Calibración inicial
  if(!calibrated){
    rollSum += roll; pitchSum += pitch; yawSum += yaw; calibrationSamples++;
    if(millis()-calibrationStart>3000){
      rollOffset=rollSum/calibrationSamples;
      pitchOffset=pitchSum/calibrationSamples;
      yawOffset=yawSum/calibrationSamples;
      calibrated=true;
      Serial.println("Calibracion completa");
    }
    return;
  }

  float rollRel=roll-rollOffset;
  float pitchRel=pitch-pitchOffset;
  float yawRel=yaw-yawOffset;

  static unsigned long lastPrint=0;
  if(millis()-lastPrint>100){
    Serial.print(rollRel); Serial.print("\t");
    Serial.print(pitchRel); Serial.print("\t");
    Serial.println(yawRel);
    lastPrint=millis();
  }
}