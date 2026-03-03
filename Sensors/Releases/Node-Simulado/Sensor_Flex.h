#ifndef FLEXOMETRO_LIB_H
#define FLEXOMETRO_LIB_H

#include <Arduino.h>

class MyFlex {
public:
    MyFlex() {}

    void begin() {
        Serial.println("Flexómetro (Simulado) listo.");
    }

    // Retorna un valor simulado de resistencia o ángulo (0 a 1023)
    int getFlexValue() {
        return random(0, 1024);
    }
};

#endif