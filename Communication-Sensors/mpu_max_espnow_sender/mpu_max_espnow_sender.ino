#include <WiFi.h>
#include <esp_now.h>

uint8_t repetidor[] = {0xC8, 0xF0, 0x9E, 0x31, 0x34, 0x70};

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

SensorPayload data;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, repetidor, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_add_peer(&peerInfo);
}

void loop() {

  strcpy(data.nodo_id, "NODO_1");
  data.timestamp = millis();

  data.imu.roll  = random(-900,900)/10.0;
  data.imu.pitch = random(-900,900)/10.0;
  data.imu.yaw   = random(-900,900)/10.0;

  data.max.ir  = random(50000,100000);
  data.max.red = random(50000,100000);

  esp_now_send(repetidor, (uint8_t*)&data, sizeof(data));

  Serial.println("Datos enviados al repetidor");
  delay(1000);
}