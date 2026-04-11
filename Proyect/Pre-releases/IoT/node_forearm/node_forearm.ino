#include <WiFi.h>
#include <esp_now.h>

#include "sensor_MPU9250.h"

// Dirección MAC del Master Node
uint8_t broadcastAddress[] = {0x8C, 0x4F, 0x00, 0xAD, 0x68, 0x6C};

// Estructura específica para el antebrazo (solo datos de orientación)
typedef struct struct_message_forearm {
    float roll_f;
    float pitch_f;
    float yaw_f;
    int node_id;  // Identificador: 2 para antebrazo
} struct_message_forearm;

struct_message_forearm myData;

// Sensor MPU9250 (usa la misma librería que el nodo brazo)
MyMPU9250 mpu;

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    Serial.print("Estado del envío: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ÉXITO" : "FALLO");
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Esperar a que el monitor serial se conecte
    
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

    // Configurar ID del nodo (2 = antebrazo)
    myData.node_id = 2;
    
    // Inicializar sensor MPU REAL
    mpu.begin();
    
    Serial.println("Nodo ANTEBRAZO iniciado - Usando sensor MPU9250 REAL");
    Serial.println("Enviando datos absolutos (roll_f, pitch_f, yaw_f)");
    Serial.println("=================================");
}

void loop() {
    // Obtener datos del MPU9250 real
    Orientation mpuData = mpu.getData();
    
    // Llenar la estructura con datos absolutos del sensor
    myData.roll_f = mpuData.roll;
    myData.pitch_f = mpuData.pitch;
    myData.yaw_f = mpuData.yaw;

    // Enviar por ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    // Mostrar valores (incluso durante calibración)
    Serial.print("[ANTEBRAZO] ID:"); Serial.print(myData.node_id);
    Serial.print(" | Roll_f: "); Serial.print(myData.roll_f, 1);
    Serial.print("° | Pitch_f: "); Serial.print(myData.pitch_f, 1);
    Serial.print("° | Yaw_f: "); Serial.print(myData.yaw_f, 1);
    Serial.println("°");
    
    if (result != ESP_OK) {
        Serial.println(" [ERROR ENVIO]");
    }

    delay(100); // 100ms para ver cambios durante calibración
}