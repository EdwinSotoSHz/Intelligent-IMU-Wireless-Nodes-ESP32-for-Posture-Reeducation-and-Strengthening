#include <WiFi.h>
#include <esp_now.h>

#include "sensor_GY91.h" 

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

MyGY91 mpu;

void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    Serial.print("Estado del envío: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ÉXITO" : "FALLO");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
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

    myData.node_id = 2;
    
    // Inicializar sensor GY-91
    mpu.begin();
    
    Serial.println("Nodo ANTEBRAZO iniciado - Usando GY-91 (MPU9250 + BMP280)");
    Serial.println("Enviando datos absolutos (roll_f, pitch_f, yaw_f)");
    Serial.println("=================================");
}

void loop() {
    Orientation mpuData = mpu.getData();
    
    myData.roll_f  = mpuData.roll;
    myData.pitch_f = mpuData.pitch;
    myData.yaw_f   = mpuData.yaw;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    Serial.print("[ANTEBRAZO] ID:"); Serial.print(myData.node_id);
    Serial.print(" | Roll_f: "); Serial.print(myData.roll_f, 1);
    Serial.print("° | Pitch_f: "); Serial.print(myData.pitch_f, 1);
    Serial.print("° | Yaw_f: "); Serial.print(myData.yaw_f, 1);
    Serial.println("°");
    
    if (result != ESP_OK) {
        Serial.println(" [ERROR ENVIO]");
    }

    delay(100);
}