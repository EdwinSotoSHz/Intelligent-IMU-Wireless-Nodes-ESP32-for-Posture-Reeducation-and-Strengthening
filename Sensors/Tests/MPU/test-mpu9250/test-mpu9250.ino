/*
| MPU9250      |  ESP32             |
| VCC          |  3.3V              |
| GND          |  GND               |
| SCL          |  GPIO32            |
| SDA          |  GPIO33            |
*/
#include <Wire.h>

#define SDA_PIN 1
#define SCL_PIN 0
#define MPU_ADDR 0x68

#define ACCEL_SCALE 16384.0
#define GYRO_SCALE 131.0

// ==== TUS BIAS MEDIDOS (Ajustados) ====
#define GYRO_BIAS_X  -554.73
#define GYRO_BIAS_Y   483.33
#define GYRO_BIAS_Z     5.64

#define ACC_BIAS_X  (644.0)   // Usa los valores crudos que mediste antes
#define ACC_BIAS_Y  (-760.0)
#define ACC_BIAS_Z  (18048.0 - 16384.0) 
// ===========================

int16_t axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw;
float roll = 0, pitch = 0, yaw = 0;
unsigned long prevTime;

void readMPU() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 14, true);
    axRaw = Wire.read()<<8|Wire.read();
    ayRaw = Wire.read()<<8|Wire.read();
    azRaw = Wire.read()<<8|Wire.read();
    Wire.read(); Wire.read();
    gxRaw = Wire.read()<<8|Wire.read();
    gyRaw = Wire.read()<<8|Wire.read();
    gzRaw = Wire.read()<<8|Wire.read();
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // MPU Init
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); Wire.write(0x00); Wire.endTransmission();

    // Sincronización inicial
    readMPU();
    float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
    float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
    float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;
    
    roll = atan2(ay, az) * 180 / PI;
    pitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;
    prevTime = micros();
    
    Serial.println("Roll, Pitch, Yaw:");
}

void loop() {
    unsigned long now = micros();
    float dt = (now - prevTime) / 1000000.0;
    prevTime = now;

    readMPU();

    // Convertir a unidades físicas corregidas
    float gx = (gxRaw - GYRO_BIAS_X) / GYRO_SCALE;
    float gy = (gyRaw - GYRO_BIAS_Y) / GYRO_SCALE;
    float gz = (gzRaw - GYRO_BIAS_Z) / GYRO_SCALE;

    float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
    float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
    float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;

    // Acelerómetro (Referencia de gravedad)
    float accRoll = atan2(ay, az) * 180 / PI;
    float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;

    // Filtro Complementario (Funde la estabilidad del Acc con la rapidez del Gyro)
    roll = 0.96 * (roll + gx * dt) + 0.04 * accRoll;
    pitch = 0.96 * (pitch + gy * dt) + 0.04 * accPitch;
    yaw += gz * dt; // El Yaw solo se puede medir con Gyro o Magnetómetro

    // Mostrar en consola cada 100ms aproximadamente
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 100) {
        Serial.print("Roll: ");
        Serial.print(roll);
        Serial.print("°\tPitch: ");
        Serial.print(pitch);
        Serial.print("°\tYaw: ");
        Serial.print(yaw);
        Serial.println("°");
        lastPrint = millis();
    }
}