#ifndef SENSOR_AD8232_H
#define SENSOR_AD8232_H

#include <Arduino.h>

class MyAD8232 {
private:
    int lastECGValue;
    int minECG;
    int maxECG;
    
public:
    MyAD8232() : lastECGValue(0), minECG(0), maxECG(4095) {}

    void begin() {
        Serial.println("AD8232 (Simulado) listo - Sensor de ECG");
        randomSeed(analogRead(0)); // Inicializar generador aleatorio
    }

    int getECG() {
        // Simular una lectura de ECG con variación realista
        // El ECG tiene un patrón característico: complejo QRS con variaciones
        int variation = random(-50, 51); // Variación aleatoria
        int baseValue = 2048; // Valor medio de 0-4095
        
        // Simular latidos cada cierto tiempo (aproximadamente 60-100 BPM)
        static unsigned long lastBeatTime = 0;
        unsigned long currentTime = millis();
        
        // Simular un pico QRS cada 600-1000ms (60-100 BPM)
        if (currentTime - lastBeatTime > random(600, 1001)) {
            lastBeatTime = currentTime;
            // Pico QRS (entre 3000 y 4000)
            lastECGValue = random(3000, 4001);
        } else {
            // Entre latidos, valor base con pequeñas variaciones
            // Onda T pequeña después del latido
            if (currentTime - lastBeatTime > 200 && currentTime - lastBeatTime < 400) {
                // Onda T (valor ligeramente elevado)
                lastECGValue = baseValue + random(100, 300) + variation/2;
            } else {
                // Línea base con pequeñas variaciones
                lastECGValue = baseValue + variation;
            }
        }
        
        // Asegurar que el valor esté dentro del rango 0-4095
        lastECGValue = constrain(lastECGValue, minECG, maxECG);
        
        return lastECGValue;
    }
};

#endif