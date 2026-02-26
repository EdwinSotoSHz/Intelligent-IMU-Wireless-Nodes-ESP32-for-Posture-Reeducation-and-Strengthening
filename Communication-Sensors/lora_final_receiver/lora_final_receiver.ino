#include <WiFi.h>
#include <esp_now.h>

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

  Serial.println("===== DATOS RECIBIDOS =====");

  Serial.print("Nodo: ");
  Serial.println(receivedData.nodo_id);

  Serial.print("Timestamp: ");
  Serial.println(receivedData.timestamp);

  Serial.print("Roll: ");
  Serial.println(receivedData.imu.roll);

  Serial.print("Pitch: ");
  Serial.println(receivedData.imu.pitch);

  Serial.print("Yaw: ");
  Serial.println(receivedData.imu.yaw);

  Serial.print("IR: ");
  Serial.println(receivedData.max.ir);

  Serial.print("RED: ");
  Serial.println(receivedData.max.red);

  Serial.println("===========================");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receptor final listo");
}

void loop() {}