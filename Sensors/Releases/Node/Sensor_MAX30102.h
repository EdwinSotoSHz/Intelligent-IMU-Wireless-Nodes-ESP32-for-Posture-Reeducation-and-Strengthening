#ifndef SENSOR_MAX30102_H
#define SENSOR_MAX30102_H

#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

struct VitalSigns {
    float spo2;
    int heartRate;
    bool stable;        // Indica si las lecturas son estables
    bool fingerPresent; // Indica si hay un dedo presente
};

class MyMAX30102 {
private:
    MAX30105 particleSensor;
    
    // Pines I2C (por defecto 8 y 4 como en tu código)
    int sda_pin;
    int scl_pin;
    
    // Constantes de configuración (tomadas de test-max30102.ino)
    const uint32_t IR_THRESHOLD = 50000;
    const int SMOOTH_SIZE = 6;
    const int MIN_VALID_SAMPLES = 3;
    const int BUFFER_LENGTH = 100;
    
    // Buffers para el algoritmo
    uint32_t irBuffer[100];
    uint32_t redBuffer[100];
    int32_t bufferLength;
    
    // Historial para suavizado
    int32_t hrHistory[6];
    int32_t spo2History[6];
    int histIndex;
    
    // Variables para resultados del algoritmo
    int32_t spo2;
    int8_t validSPO2;
    int32_t heartRate;
    int8_t validHeartRate;
    
    // Flags internos
    bool firstRead;
    
    // ==========================================
    // MÉTODOS PRIVADOS (TOMADOS DE test-max30102.ino)
    // ==========================================
    
    // Verifica si hay dedo promediando las últimas 5 muestras
    bool fingerPresent() {
        uint32_t avg = 0;
        for (int i = 95; i < 100; i++) avg += irBuffer[i];
        return (avg / 5) > IR_THRESHOLD;
    }
    
    // Recolecta un rango de muestras del sensor
    void collectSamples(byte start, byte end) {
        for (byte i = start; i < end; i++) {
            while (particleSensor.available() == false) particleSensor.check();
            redBuffer[i] = particleSensor.getRed();
            irBuffer[i]  = particleSensor.getIR();
            particleSensor.nextSample();
        }
    }
    
    // Reinicia los historiales (exactamente como en test-max30102.ino)
    void resetHistories() {
        memset(hrHistory,   0, sizeof(hrHistory));
        memset(spo2History, 0, sizeof(spo2History));
        histIndex = 0;
    }
    
    // Calcula la media ignorando ceros (como en test-max30102.ino)
    int32_t smoothedValue(int32_t* history, int* validCount) {
        int32_t sum = 0;
        *validCount = 0;
        for (int i = 0; i < SMOOTH_SIZE; i++) {
            if (history[i] > 0) { 
                sum += history[i]; 
                (*validCount)++; 
            }
        }
        return (*validCount) > 0 ? sum / (*validCount) : 0;
    }

public:
    // ==========================================
    // CONSTRUCTOR
    // ==========================================
    MyMAX30102(int sda = 8, int scl = 4) : 
        sda_pin(sda), 
        scl_pin(scl),
        firstRead(true)
    {
        bufferLength = BUFFER_LENGTH;
    }
    
    // ==========================================
    // INICIALIZACIÓN (begin)
    // ==========================================
    bool begin() {
        Wire.begin(sda_pin, scl_pin);
        
        if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
            return false;  // Sensor no detectado
        }
        
        // Configuración IDÉNTICA a test-max30102.ino:
        // particleSensor.setup(50, 4, 2, 100, 411, 4096);
        particleSensor.setup(50, 4, 2, 100, 411, 4096);
        
        resetHistories();
        firstRead = true;
        
        return true;
    }
    
    // ==========================================
    // MÉTODO PRINCIPAL (getVitals)
    // ==========================================
    VitalSigns getVitals() {
        VitalSigns vitals;
        vitals.spo2 = 0;
        vitals.heartRate = 0;
        vitals.stable = false;
        vitals.fingerPresent = false;
        
        // --- 1. Verificación rápida de dedo (como en test-max30102.ino) ---
        // Colectamos una muestra para ver si hay dedo
        while (particleSensor.available() == false) particleSensor.check();
        uint32_t quickIR = particleSensor.getIR();
        particleSensor.nextSample();
        
        // Si no hay dedo, reiniciamos y salimos
        if (quickIR <= IR_THRESHOLD) {
            resetHistories();
            firstRead = true;
            vitals.fingerPresent = false;
            return vitals;
        }
        
        vitals.fingerPresent = true;
        
        // --- 2. Gestión del buffer (IDÉNTICO a test-max30102.ino) ---
        if (firstRead) {
            // Primera lectura: llenar buffer completo (4 segundos)
            collectSamples(0, BUFFER_LENGTH);
            firstRead = false;
        } else {
            // Lecturas subsecuentes: desplazar y agregar 25 nuevas muestras
            for (byte i = 25; i < BUFFER_LENGTH; i++) {
                redBuffer[i - 25] = redBuffer[i];
                irBuffer[i - 25]  = irBuffer[i];
            }
            
            // Recolectar 25 nuevas muestras (posiciones 75-99)
            collectSamples(75, BUFFER_LENGTH);
        }
        
        // --- 3. Verificación robusta de dedo (con buffer lleno) ---
        if (!fingerPresent()) {
            resetHistories();
            firstRead = true;
            vitals.fingerPresent = false;
            return vitals;
        }
        
        // --- 4. Ejecutar algoritmo de SpO2 (IDÉNTICO a test-max30102.ino) ---
        maxim_heart_rate_and_oxygen_saturation(
            irBuffer, bufferLength, redBuffer,
            &spo2, &validSPO2, &heartRate, &validHeartRate
        );
        
        // --- 5. Validación fisiológica y actualización de historial ---
        // (Exactamente como en test-max30102.ino)
        bool hrOk   = validHeartRate && heartRate >= 45 && heartRate <= 149;
        bool spo2Ok = validSPO2 && spo2 >= 85 && spo2 <= 100;
        
        if (hrOk)   hrHistory[histIndex]   = heartRate;
        else        hrHistory[histIndex]   = 0;
        
        if (spo2Ok) spo2History[histIndex] = spo2;
        else        spo2History[histIndex] = 0;
        
        histIndex = (histIndex + 1) % SMOOTH_SIZE;
        
        // --- 6. Calcular valores suavizados ---
        int validHRCount, validSPO2Count;
        int32_t displayHR   = smoothedValue(hrHistory,   &validHRCount);
        int32_t displaySPO2 = smoothedValue(spo2History, &validSPO2Count);
        
        // --- 7. Determinar estabilidad (como en test-max30102.ino) ---
        bool stable = (validHRCount >= MIN_VALID_SAMPLES) && 
                      (validSPO2Count >= MIN_VALID_SAMPLES);
        
        // --- 8. Asignar valores a la estructura de retorno ---
        vitals.heartRate = (displayHR > 0) ? (int)displayHR : 0;
        vitals.spo2 = (displaySPO2 > 0) ? (float)displaySPO2 : 0.0;
        vitals.stable = stable;
        vitals.fingerPresent = true;
        
        return vitals;
    }
    
    // ==========================================
    // MÉTODOS ADICIONALES (UTILIDADES)
    // ==========================================
    
    // Verificación rápida de dedo sin procesar (útil para loops externos)
    bool isFingerPresent() {
        if (particleSensor.available()) {
            uint32_t ir = particleSensor.getIR();
            particleSensor.nextSample();
            return ir > IR_THRESHOLD;
        }
        return false;
    }
    
    // Reiniciar manualmente el estado del sensor
    void reset() {
        resetHistories();
        firstRead = true;
    }
    
    // Obtener la configuración actual (para depuración)
    void printConfig() {
        Serial.println(F("=== Configuración MAX30102 ==="));
        Serial.print(F("IR Threshold: ")); Serial.println(IR_THRESHOLD);
        Serial.print(F("Smooth Size: ")); Serial.println(SMOOTH_SIZE);
        Serial.print(F("Min Valid Samples: ")); Serial.println(MIN_VALID_SAMPLES);
        Serial.print(F("Buffer Length: ")); Serial.println(BUFFER_LENGTH);
        Serial.println(F("=============================="));
    }
};

#endif