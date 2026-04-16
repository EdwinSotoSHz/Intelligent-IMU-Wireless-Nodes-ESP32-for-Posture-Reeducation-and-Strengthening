#ifndef SENSOR_GY91_H
#define SENSOR_GY91_H

#include <Arduino.h>
#include <Wire.h>
#include <math.h>

struct Orientation {
    float yaw;
    float pitch;
    float roll;
};

class MyGY91 {
private:
    const int SDA_PIN = 1;
    const int SCL_PIN = 0;

    const int MPU_ADDR = 0x68;
    const int BMP_ADDR = 0x76;

    const float ACCEL_SCALE = 16384.0;
    const float GYRO_SCALE = 131.0;

    // Bias (los puedes recalibrar luego)
    const float GYRO_BIAS_X = -586.58;
    const float GYRO_BIAS_Y = 503.27;
    const float GYRO_BIAS_Z = 67.01;

    const float ACC_BIAS_X = 1477.39;
    const float ACC_BIAS_Y = 379.93;
    const float ACC_BIAS_Z = 1670.55;

    int16_t axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw;

    float roll = 0, pitch = 0, yaw = 0;
    float rollOffset = 0, pitchOffset = 0;

    bool calibrated = false;
    bool firstRead = true;

    unsigned long prevTime = 0;
    unsigned long calibrationStart = 0;

    float rollSum = 0, pitchSum = 0;
    int calibrationSamples = 0;

    // BMP280 (opcional)
    float temperature = 0;
    float pressure = 0;

    void readMPU() {
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x3B);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, 14, true);

        axRaw = Wire.read() << 8 | Wire.read();
        ayRaw = Wire.read() << 8 | Wire.read();
        azRaw = Wire.read() << 8 | Wire.read();
        Wire.read(); Wire.read();
        gxRaw = Wire.read() << 8 | Wire.read();
        gyRaw = Wire.read() << 8 | Wire.read();
        gzRaw = Wire.read() << 8 | Wire.read();
    }

    float normalizeAngle(float angle) {
        while (angle > 180) angle -= 360;
        while (angle < -180) angle += 360;
        return angle;
    }

public:
    void begin() {
        Serial.println("Iniciando GY-91...");

        Wire.begin(SDA_PIN, SCL_PIN);

        // Wake MPU9250
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x6B);
        Wire.write(0x00);
        Wire.endTransmission();

        delay(100);

        readMPU();

        float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
        float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
        float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;

        // 🔧 Normalización (mejora estabilidad)
        float norm = sqrt(ax*ax + ay*ay + az*az);
        ax /= norm;
        ay /= norm;
        az /= norm;

        roll = atan2(ay, az) * 180.0 / PI;
        pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
        yaw = 0;

        prevTime = micros();
        calibrationStart = millis();

        firstRead = true;

        Serial.println("Calibrando 3 segundos...");
    }

    Orientation getData() {
        unsigned long now = micros();
        float dt = (now - prevTime) / 1000000.0;
        prevTime = now;

        if (dt <= 0 || dt > 0.1) dt = 0.01;

        readMPU();

        float gx = (gxRaw - GYRO_BIAS_X) / GYRO_SCALE;
        float gy = (gyRaw - GYRO_BIAS_Y) / GYRO_SCALE;
        float gz = (gzRaw - GYRO_BIAS_Z) / GYRO_SCALE;

        float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
        float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
        float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;

        // 🔧 Normalización
        float norm = sqrt(ax*ax + ay*ay + az*az);
        ax /= norm;
        ay /= norm;
        az /= norm;

        float accRoll = atan2(ay, az) * 180.0 / PI;
        float accPitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

        if (firstRead) {
            roll = accRoll;
            pitch = accPitch;
            yaw = 0;
            firstRead = false;
        } else {
            float alpha = 0.98;

            roll = normalizeAngle(alpha * (roll + gx * dt) + (1 - alpha) * accRoll);
            pitch = normalizeAngle(alpha * (pitch + gy * dt) + (1 - alpha) * accPitch);

            yaw += gz * dt;
            yaw = normalizeAngle(yaw);
        }

        Orientation data;

        if (!calibrated) {
            rollSum += roll;
            pitchSum += pitch;
            calibrationSamples++;

            if (millis() - calibrationStart > 3000) {
                rollOffset = rollSum / calibrationSamples;
                pitchOffset = pitchSum / calibrationSamples;
                calibrated = true;

                Serial.println("Calibración completa");
            }

            data.roll = roll;
            data.pitch = pitch;
            data.yaw = yaw;

        } else {
            data.roll = roll - rollOffset;
            data.pitch = pitch - pitchOffset;
            data.yaw = yaw; // ⚠️ sin offset (evita drift falso)
        }

        return data;
    }
};

#endif