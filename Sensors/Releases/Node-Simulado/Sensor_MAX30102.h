#ifndef MAX30102_LIB_H
#define MAX30102_LIB_H

#include <Arduino.h>

struct VitalSigns {
    float spo2;
    int heartRate;
};

class MyMAX30102 {
public:
    MyMAX30102() {}

    void begin() {
        Serial.println("MAX30102 (Simulado) listo.");
    }

    VitalSigns getVitals() {
        VitalSigns vitals;
        // SpO2: Valores normales entre 95% y 100%
        vitals.spo2 = (random(950, 1000) / 10.0);
        // Heart Rate: Valores entre 60 y 110 BPM
        vitals.heartRate = random(60, 110);
        return vitals;
    }
};

#endif