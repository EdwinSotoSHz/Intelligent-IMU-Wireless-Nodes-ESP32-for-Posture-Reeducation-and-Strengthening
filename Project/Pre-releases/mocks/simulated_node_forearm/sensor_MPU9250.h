#ifndef SENSOR_MPU9250_H
#define SENSOR_MPU9250_H

#include <Arduino.h>

// Estructura para organizar los datos de orientación
struct Orientation {
    float yaw;
    float pitch;
    float roll;
};

class MyMPU9250 {
public:
    // Constructor
    MyMPU9250() {}

    // Inicialización (Simulada)
    void begin() {
        // Aquí iría el mpu.setup() real
        Serial.println("MPU9250 (Simulado) iniciado correctamente.");
    }

    // Método que retorna la estructura con números aleatorios
    Orientation getData() {
        Orientation data;
        
        // Generamos valores aleatorios para pruebas
        // Yaw: 0 a 360 | Pitch/Roll: -180 a 180
        data.yaw   = random(0, 10000) / 10000.0 * (90 - (-90)) + (-90);
        data.pitch = random(0, 10000) / 10000.0 * (90 - (-90)) + (-90);
        data.roll  = random(0, 10000) / 10000.0 * (90 - (-90)) + (-90);
        
        return data;
    }
};

#endif