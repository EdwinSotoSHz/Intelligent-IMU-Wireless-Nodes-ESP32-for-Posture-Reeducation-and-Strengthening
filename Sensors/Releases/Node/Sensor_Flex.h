#ifndef SENSOR_FLEX_H
#define SENSOR_FLEX_H

#include <Arduino.h>

class MyFlex {
private:
    int flexPin;           // Pin analógico donde está conectado el flexómetro
    int minRawValue;        // Valor mínimo esperado (en reposo, sin flexionar)
    int maxRawValue;        // Valor máximo esperado (máxima flexión)
    bool calibrated;        // Indica si el sensor ha sido calibrado
    
    // Valores por defecto para calibración (se actualizarán durante el uso)
    const int DEFAULT_MIN = 0;
    const int DEFAULT_MAX = 4095;  // ADC de ESP32 es de 12 bits (0-4095)
    
public:
    // ==========================================
    // CONSTRUCTOR
    // ==========================================
    MyFlex(int pin = 3) : 
        flexPin(pin), 
        minRawValue(4095),  // Inicializamos al revés para calibración
        maxRawValue(0), 
        calibrated(false) 
    {}
    
    // ==========================================
    // INICIALIZACIÓN
    // ==========================================
    void begin() {
        // Configurar el pin como entrada (por defecto ya lo es)
        pinMode(flexPin, INPUT);
        
        Serial.print("Flexómetro inicializado en pin GPIO ");
        Serial.println(flexPin);
        Serial.println("Moviendo el sensor para calibración automática...");
    }
    
    // ==========================================
    // LECTURA CRUDA DEL ADC
    // ==========================================
    // Retorna el valor analógico directamente del ADC (0-4095)
    int getRawValue() {
        return analogRead(flexPin);
    }
    
    // ==========================================
    // LECTURA CON CALIBRACIÓN AUTOMÁTICA
    // ==========================================
    // Retorna un valor calibrado de 0 a 1023 (para compatibilidad con la versión simulada)
    // Además, actualiza automáticamente los rangos mínimo y máximo
    int getFlexValue() {
        int rawValue = analogRead(flexPin);
        
        // Calibración automática: actualizar mínimos y máximos
        if (rawValue < minRawValue) {
            minRawValue = rawValue;
            calibrated = true;  // Ya tenemos al menos un valor
        }
        if (rawValue > maxRawValue) {
            maxRawValue = rawValue;
            calibrated = true;
        }
        
        // Si no hay calibración aún, retornar el valor crudo mapeado a 0-1023
        if (!calibrated || maxRawValue <= minRawValue) {
            return map(rawValue, 0, 4095, 0, 1023);
        }
        
        // Mapear el valor dentro del rango calibrado a 0-1023
        // Nota: constrain evita valores fuera de rango por ruido
        int constrainedValue = constrain(rawValue, minRawValue, maxRawValue);
        return map(constrainedValue, minRawValue, maxRawValue, 0, 1023);
    }
    
    // ==========================================
    // OBTENER PORCENTAJE DE FLEXIÓN
    // ==========================================
    // Retorna el porcentaje de flexión (0% = reposo, 100% = máxima flexión)
    int getFlexPercent() {
        if (!calibrated || maxRawValue <= minRawValue) {
            return 0;  // No hay calibración suficiente
        }
        
        int rawValue = analogRead(flexPin);
        int constrainedValue = constrain(rawValue, minRawValue, maxRawValue);
        
        // Calcular porcentaje
        float percent = (float)(constrainedValue - minRawValue) / 
                       (maxRawValue - minRawValue) * 100.0;
        
        return (int)percent;
    }
    
    // ==========================================
    // MÉTODOS DE CALIBRACIÓN MANUAL
    // ==========================================
    
    // Calibrar el valor mínimo (flexión en reposo)
    void calibrateMin() {
        minRawValue = analogRead(flexPin);
        calibrated = true;
        Serial.print("Mínimo calibrado: ");
        Serial.println(minRawValue);
    }
    
    // Calibrar el valor máximo (máxima flexión)
    void calibrateMax() {
        maxRawValue = analogRead(flexPin);
        calibrated = true;
        Serial.print("Máximo calibrado: ");
        Serial.println(maxRawValue);
    }
    
    // Reiniciar calibración
    void resetCalibration() {
        minRawValue = 4095;
        maxRawValue = 0;
        calibrated = false;
        Serial.println("Calibración reiniciada");
    }
    
    // ==========================================
    // MÉTODOS DE CONFIGURACIÓN
    // ==========================================
    
    // Establecer rangos manualmente (si se conocen)
    void setRange(int minVal, int maxVal) {
        minRawValue = minVal;
        maxRawValue = maxVal;
        calibrated = true;
    }
    
    // Obtener el rango actual
    void getRange(int &minVal, int &maxVal) {
        minVal = minRawValue;
        maxVal = maxRawValue;
    }
    
    // ==========================================
    // MÉTODOS DE ESTADO
    // ==========================================
    
    bool isCalibrated() {
        return calibrated;
    }
};

#endif