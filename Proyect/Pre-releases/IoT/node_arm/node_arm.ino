#include <WiFi.h>
#include <esp_now.h>

#include "sensor_MPU9250.h"
#include "sensor_AD8232.h"

// Dirección MAC del Master Node
uint8_t broadcastAddress[] = {0x8C, 0x4F, 0x00, 0xAD, 0x68, 0x6C};

// Estructura específica para el brazo
typedef struct struct_message_arm {
    float roll_a;
    float pitch_a;
    float yaw_a;
    int ecg;
    int node_id;
} struct_message_arm;

struct_message_arm myData;

// Sensores
MyMPU9250 mpu;
MyAD8232 ecgSensor;

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

    myData.node_id = 1;

    // Inicializar sensores REALES
    mpu.begin();
    ecgSensor.begin();
    
    Serial.println("Nodo BRAZO iniciado - Usando sensores REALES");
    Serial.println("=================================");
}

void loop() {
    Orientation mpuData = mpu.getData();
    
    myData.roll_a = mpuData.roll;
    myData.pitch_a = mpuData.pitch;
    myData.yaw_a = mpuData.yaw;
    myData.ecg = ecgSensor.getECG();

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    // Mostrar siempre los valores (incluso durante calibración)
    Serial.print("[BRAZO] ID:"); Serial.print(myData.node_id);
    Serial.print(" | Roll_a: "); Serial.print(myData.roll_a, 1);
    Serial.print("° | Pitch_a: "); Serial.print(myData.pitch_a, 1);
    Serial.print("° | Yaw_a: "); Serial.print(myData.yaw_a, 1);
    Serial.print("° | ECG: "); Serial.println(myData.ecg);
    
    if (result != ESP_OK) {
        Serial.println(" [ERROR ENVIO]");
    }

    delay(100); // Reducido a 100ms para ver cambios más rápido durante calibración
}