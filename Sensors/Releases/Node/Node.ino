#include <WiFi.h>
#include <esp_now.h>

// CAMBIA ESTAS TRES LÍNEAS:
// #include "Sensor_MPU9250.h"      // <-- Versión simulada (quitar)
// #include "Sensor_Flex.h"          // <-- Versión simulada (quitar)  
// #include "Sensor_MAX30102.h"      // <-- Versión simulada (quitar)

// POR ESTAS (los nuevos headers que acabamos de crear):
#include "Sensor_MPU9250.h"   // <-- Versión REAL (con MPU9250)
#include "Sensor_Flex.h"      // <-- Versión REAL (con flexómetro)  
#include "Sensor_MAX30102.h"  // <-- Versión REAL (con MAX30102)

// El RESTO del código queda IGUAL - NO CAMBIES NADA MÁS
// ...

// Dirección MAC del receptor (NO CAMBIAR)
uint8_t broadcastAddress[] = {0xC8, 0xF0, 0x9E, 0x31, 0x34, 0x70};

// Estructura completa con todos los sensores (NO CAMBIAR)
typedef struct struct_message {
    float yaw, pitch, roll;
    int flex;
    float spo2;
    int heartRate;
} struct_message;

struct_message myData;

// Sensores (ahora con los nuevos headers REALES)
MyMPU9250 mpu;        // Usa pines 32,33 por defecto
MyFlex flex(4);       // GPIO4 para el flexómetro
MyMAX30102 oximetro;  // Usa pines 8,4 por defecto

// Firma correcta para el emisor (NO CAMBIAR)
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    Serial.print("Estado del envío: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ÉXITO" : "FALLO");
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error agregando peer");
        return;
    }

    // Inicializar sensores REALES
    mpu.begin();           // MPU9250 en pines 32,33
    flex.begin();          // Flex en GPIO4
    oximetro.begin();      // MAX30102 en pines 8,4
}

void loop() {
    // Obtener datos de sensores REALES
    Orientation mpuData = mpu.getData();
    VitalSigns salud = oximetro.getVitals();

    // Llenar la estructura (IGUAL QUE ANTES)
    myData.yaw = mpuData.yaw;
    myData.pitch = mpuData.pitch;
    myData.roll = mpuData.roll;
    myData.flex = flex.getFlexValue();  // Devuelve 0-1023
    myData.spo2 = salud.spo2;
    myData.heartRate = salud.heartRate;

    // Enviar por ESP-NOW (IGUAL QUE ANTES)
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
        Serial.println("Enviando paquete completo...");
        
        // Mostrar datos para verificar
        Serial.print("Yaw: "); Serial.print(myData.yaw);
        Serial.print(" Pitch: "); Serial.print(myData.pitch);
        Serial.print(" Roll: "); Serial.println(myData.roll);
        
        Serial.print("Flex: "); Serial.print(myData.flex);
        Serial.print(" SpO2: "); Serial.print(myData.spo2);
        Serial.print(" HR: "); Serial.println(myData.heartRate);
        
    } else {
        Serial.println("Error enviando datos");
    }

    delay(1000); 
}