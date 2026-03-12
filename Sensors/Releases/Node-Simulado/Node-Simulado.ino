#include <WiFi.h>
#include <esp_now.h>

#include "Sensor_MPU9250.h"
#include "Sensor_Flex.h"
#include "Sensor_MAX30102.h"

// Dirección MAC del receptor
uint8_t broadcastAddress[] = {0x8C, 0x4F, 0x00, 0xAD, 0x68, 0x6C};

// Estructura completa con todos los sensores
typedef struct struct_message {
    float yaw, pitch, roll;
    int flex;
    float spo2;
    int heartRate;
} struct_message;

struct_message myData;

// Sensores simulados
MyMPU9250 mpu;
MyFlex flex;
MyMAX30102 oximetro;

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

    // Inicializar simuladores
    mpu.begin();
    flex.begin();
    oximetro.begin();
}

void loop() {
    // Obtener datos simulados
    Orientation mpuData = mpu.getData();
    VitalSigns salud = oximetro.getVitals();

    // Llenar la estructura
    myData.yaw = mpuData.yaw;
    myData.pitch = mpuData.pitch;
    myData.roll = mpuData.roll;
    myData.flex = flex.getFlexValue();
    myData.spo2 = salud.spo2;
    myData.heartRate = salud.heartRate;

    // Enviar por ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

    if (result == ESP_OK) {
        Serial.println("Enviando paquete completo...");
    } else {
        Serial.println("Error enviando datos");
    }

    delay(1000); 
}