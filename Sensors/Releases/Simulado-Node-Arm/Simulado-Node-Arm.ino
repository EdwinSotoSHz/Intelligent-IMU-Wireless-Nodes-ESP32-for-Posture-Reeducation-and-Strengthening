#include <WiFi.h>
#include <esp_now.h>

#include "Sensor_MPU9250.h"

// Dirección MAC del Master Node
uint8_t broadcastAddress[] = {0x8C, 0x4F, 0x00, 0xAD, 0x68, 0x6C};

// Estructura específica para el brazo (solo datos absolutos)
typedef struct struct_message_arm {
    float roll_a;
    float pitch_a;
    float yaw_a;
    int node_id;  // Identificador para que el Master sepa qué nodo es
} struct_message_arm;

struct_message_arm myData;

// Sensor MPU9250
MyMPU9250 mpu;

// Firma correcta para el emisor
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

    // Configurar ID del nodo (1 = Brazo)
    myData.node_id = 1;
    
    // Inicializar sensor MPU
    mpu.begin();
    
    Serial.println("Nodo BRAZO iniciado - Enviando datos absolutos (roll_a, pitch_a, yaw_a)");
    Serial.println("=================================");
}

void loop() {
    // Obtener datos del MPU9250
    Orientation mpuData = mpu.getData();
    
    // Llenar la estructura solo con datos absolutos
    myData.roll_a = mpuData.roll;
    myData.pitch_a = mpuData.pitch;
    myData.yaw_a = mpuData.yaw;

    // Enviar por ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
        Serial.print("[BRAZO] Enviado - ID:"); Serial.print(myData.node_id);
        Serial.print(" | Roll_a: "); Serial.print(myData.roll_a, 1);
        Serial.print("° | Pitch_a: "); Serial.print(myData.pitch_a, 1);
        Serial.print("° | Yaw_a: "); Serial.print(myData.yaw_a, 1);
        Serial.println("°");
    } else {
        Serial.println("[BRAZO] Error enviando datos");
    }

    delay(550); // Frecuencia alta para datos de orientación
}