#ifndef SENSOR_AD8232_H
#define SENSOR_AD8232_H

#include <Arduino.h>

class MyAD8232 {
private:
    // Pines para AD8232
    const int PIN_ECG = 2;   // Salida analógica
    const int PIN_LOP = 3;   // Leads OFF detect - (detecta desconexión)
    const int PIN_LON = 5;   // Leads OFF detect + (antes era el 5)
    
    int lastECGValue;
    int minECG;
    int maxECG;
    
public:
    MyAD8232() : lastECGValue(0), minECG(0), maxECG(4095) {}
    
    void begin() {
        Serial.println("Iniciando AD8232 real...");
        
        pinMode(PIN_ECG, INPUT);
        pinMode(PIN_LOP, INPUT);
        pinMode(PIN_LON, INPUT);
        
        Serial.println("AD8232 listo - Sensor de ECG real");
    }
    
    int getECG() {
        // Leer señal ECG
        int ecg = analogRead(PIN_ECG);
        
        // Verificar desconexión de electrodos
        if (digitalRead(PIN_LOP) == HIGH || digitalRead(PIN_LON) == HIGH) {
            lastECGValue = 0;  // Señal 0 si están desconectados
        } else {
            lastECGValue = ecg;
        }
        
        return lastECGValue;
    }
    
    // Método adicional para verificar si los electrodos están conectados
    bool isLeadsOff() {
        return (digitalRead(PIN_LOP) == HIGH || digitalRead(PIN_LON) == HIGH);
    }
};

#endif