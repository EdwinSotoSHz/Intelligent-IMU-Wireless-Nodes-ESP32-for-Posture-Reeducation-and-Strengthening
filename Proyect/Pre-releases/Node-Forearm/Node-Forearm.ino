#include <WiFi.h>
#include <esp_now.h>

#include "Sensor_MPU9250.h"
#include "Sensor_AD8232.h"

// Dirección MAC del Master Node
uint8_t broadcastAddress[] = {0x8C, 0x4F, 0x00, 0xAD, 0x68, 0x6C};

// Estructura específica para el antebrazo (datos filtrados + ECG)
typedef struct struct_message_forearm {
    float roll_f;
    float pitch_f;
    float yaw_f;
    int ecg;
    int node_id;  // Identificador para que el Master sepa qué nodo es
} struct_message_forearm;

struct_message_forearm myData;

// Sensores
MyMPU9250 mpu;
MyAD8232 ecgSensor;

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

    // Configurar ID del nodo (2 = Antebrazo)
    myData.node_id = 2;

    // Inicializar sensores
    mpu.begin();
    ecgSensor.begin();
    
    Serial.println("Nodo ANTEBRAZO iniciado - Enviando datos filtrados (roll_f, pitch_f, yaw_f) y ECG");
    Serial.println("=================================");
}

void loop() {
    // Obtener datos de los sensores
    Orientation mpuData = mpu.getData();
    
    // Llenar la estructura del antebrazo
    myData.roll_f = mpuData.roll;
    myData.pitch_f = mpuData.pitch;
    myData.yaw_f = mpuData.yaw;
    myData.ecg = ecgSensor.getECG();

    // Enviar por ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
        Serial.print("[ANTEBRAZO] Enviado - ID:"); Serial.print(myData.node_id);
        Serial.print(" | Roll_f: "); Serial.print(myData.roll_f, 1);
        Serial.print("° | Pitch_f: "); Serial.print(myData.pitch_f, 1);
        Serial.print("° | Yaw_f: "); Serial.print(myData.yaw_f, 1);
        Serial.print("° | ECG: "); Serial.println(myData.ecg);
    } else {
        Serial.println("[ANTEBRAZO] Error enviando datos");
    }

    delay(550); // 
}