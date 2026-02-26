#include <WiFi.h>
#include <esp_now.h>

uint8_t receptor[] = {0x8C, 0x4F, 0x00, 0xAD, 0x68, 0x6C};

struct IMUData {
  float roll;
  float pitch;
  float yaw;
};

struct MAXData {
  uint32_t ir;
  uint32_t red;
};

struct SensorPayload {
  char nodo_id[20];
  unsigned long timestamp;
  IMUData imu;
  MAXData max;
};

SensorPayload receivedData;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {

  memcpy(&receivedData, incomingData, sizeof(SensorPayload));

  Serial.println("Recibido desde sensores");
  Serial.println("Reenviando al receptor final...");

  esp_now_send(receptor, (uint8_t*)&receivedData, sizeof(SensorPayload));
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receptor, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_add_peer(&peerInfo);

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Repetidor listo");
}

void loop() {}