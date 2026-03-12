#include <WiFi.h>
#include <esp_now.h>

#include "Sensor_MPU9250.h"

// Dirección MAC del receptor (puedes cambiarla según necesites)
uint8_t broadcastAddress[] = {0x8C, 0x4F, 0x00, 0xAD, 0x68, 0x6C};

// Nueva estructura del payload solo con datos del MPU
typedef struct struct_message {
    float roll_a;
    float pitch_a;
    float yaw_a;
} struct_message;

struct_message myData;

// Sensor MPU9250
MyMPU9250 mpu;

// Firma correcta para el emisor en versiones recientes
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

    // Inicializar sensor MPU
    mpu.begin();
    
    Serial.println("Nodo MPU9250 iniciado - Enviando solo datos de orientación");
}

void loop() {
    // Obtener datos del MPU9250
    Orientation mpuData = mpu.getData();
    
    // Llenar la estructura solo con datos del MPU
    myData.roll_a = mpuData.roll;
    myData.pitch_a = mpuData.pitch;
    myData.yaw_a = mpuData.yaw;

    // Enviar por ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
        Serial.println("Datos MPU enviados:");
        Serial.print("  Roll_a: "); Serial.print(myData.roll_a);
        Serial.print("° | Pitch_a: "); Serial.print(myData.pitch_a);
        Serial.print("° | Yaw_a: "); Serial.print(myData.yaw_a);
        Serial.println("°");
    } else {
        Serial.println("Error enviando datos MPU");
    }

    delay(500); // Envío más frecuente para datos de orientación (500ms)
}