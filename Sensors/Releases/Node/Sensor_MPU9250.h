#ifndef SENSOR_MPU9250_H
#define SENSOR_MPU9250_H

#include <Arduino.h>
#include <Wire.h>

// Estructura para organizar los datos de orientación
struct Orientation {
    float yaw;
    float pitch;
    float roll;
};

class MyMPU9250 {
private:
    // Pines I2C por defecto (pueden cambiarse en begin)
    int sda_pin;
    int scl_pin;
    
    // Dirección I2C del MPU9250
    const int MPU_ADDR = 0x68;
    
    // Escalas del sensor
    const float ACCEL_SCALE = 16384.0;
    const float GYRO_SCALE = 131.0;
    
    // BIAS medidos (ajustados según tu calibración)
    const float GYRO_BIAS_X = -554.73;
    const float GYRO_BIAS_Y = 483.33;
    const float GYRO_BIAS_Z = 5.64;
    
    const float ACC_BIAS_X = 644.0;
    const float ACC_BIAS_Y = -760.0;
    const float ACC_BIAS_Z = 18048.0 - 16384.0; // Ajuste calculado
    
    // Variables internas para el filtro complementario
    float roll = 0;
    float pitch = 0;
    float yaw = 0;
    unsigned long prevTime;
    
    // Variables para almacenar lecturas crudas
    int16_t axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw;
    
    // Método privado para leer el MPU
    void readMPU() {
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x3B);
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, 14, true);
        
        axRaw = Wire.read() << 8 | Wire.read();
        ayRaw = Wire.read() << 8 | Wire.read();
        azRaw = Wire.read() << 8 | Wire.read();
        
        // Saltar temperatura
        Wire.read(); Wire.read();
        
        gxRaw = Wire.read() << 8 | Wire.read();
        gyRaw = Wire.read() << 8 | Wire.read();
        gzRaw = Wire.read() << 8 | Wire.read();
    }

public:
    // Constructor
    MyMPU9250() : sda_pin(1), scl_pin(0) {} // Pines por defecto de tu código
    
    // Inicialización con posibilidad de cambiar pines I2C
    void begin(int sda = 32, int scl = 33) {
        sda_pin = sda;
        scl_pin = scl;
        
        // Iniciar I2C
        Wire.begin(sda_pin, scl_pin);
        
        // Inicializar MPU
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x6B);  // Registro de gestión de energía
        Wire.write(0x00);  // Activar sensor
        Wire.endTransmission();
        
        delay(100); // Pequeña pausa para estabilización
        
        // Lectura inicial para obtener valores estables
        readMPU();
        
        // Calcular ángulos iniciales con el acelerómetro
        float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
        float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
        float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;
        
        roll = atan2(ay, az) * 180 / PI;
        pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / PI;
        
        // Inicializar tiempo para el filtro
        prevTime = micros();
        
        Serial.println("MPU9250 iniciado correctamente con lecturas reales.");
    }
    
    // Método que retorna la estructura con los ángulos actuales
    Orientation getData() {
        unsigned long now = micros();
        float dt = (now - prevTime) / 1000000.0;
        prevTime = now;
        
        // Leer datos del sensor
        readMPU();
        
        // Convertir a unidades físicas corregidas con BIAS
        float gx = (gxRaw - GYRO_BIAS_X) / GYRO_SCALE;
        float gy = (gyRaw - GYRO_BIAS_Y) / GYRO_SCALE;
        float gz = (gzRaw - GYRO_BIAS_Z) / GYRO_SCALE;
        
        float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
        float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
        float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;
        
        // Calcular ángulos del acelerómetro (referencia de gravedad)
        float accRoll = atan2(ay, az) * 180 / PI;
        float accPitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / PI;
        
        // Filtro Complementario (funde estabilidad del Acc con rapidez del Gyro)
        // Coeficientes: 0.96 para giroscopio, 0.04 para acelerómetro
        roll = 0.96 * (roll + gx * dt) + 0.04 * accRoll;
        pitch = 0.96 * (pitch + gy * dt) + 0.04 * accPitch;
        yaw += gz * dt; // El Yaw solo con giroscopio (sin magnetómetro)
        
        // Preparar y retornar los datos
        Orientation data;
        data.roll = roll;
        data.pitch = pitch;
        data.yaw = yaw;
        
        return data;
    }
    
    // Método adicional para obtener solo los datos crudos (útil para depuración)
    void getRawData(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz) {
        readMPU();
        ax = axRaw;
        ay = ayRaw;
        az = azRaw;
        gx = gxRaw;
        gy = gyRaw;
        gz = gzRaw;
    }
    
    // Método para reiniciar la orientación (poner yaw a 0, etc.)
    void resetOrientation() {
        roll = 0;
        pitch = 0;
        yaw = 0;
        prevTime = micros();
    }
};

#endif