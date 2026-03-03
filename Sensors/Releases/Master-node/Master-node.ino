#include <WiFi.h>
#include <esp_now.h>
#include <LoRa.h>
#include <SPI.h>

// ==========================================
// CONFIGURACIÓN LORA (Pines del SX1278)
// Basado en tu archivo Sender.ino
// ==========================================
const int loraRST = 23;
const int loraDI0 = 4;
const int loraNSS = 15;
const int loraMOSI = 32;
const int loraMISO = 35;
const int loraSCK = 33;
int SyncWord = 0x22; // Palabra de sincronización para filtrar ruido

// PROTOCOLO DE FRAMES LORA (IDÉNTICO AL SENDER)
byte dir_local   = 0xA1; // Mi dirección (Remitente)
byte dir_destino = 0xB2; // Dirección del receptor final (Ej. Estación base)
byte id_msjLoRa = 0;      // Número de secuencia del mensaje LoRa
byte bufferPaquete[64];   // Buffer para armar el frame LoRa completo

// ==========================================
// ESTRUCTURA DE DATOS (COMPARTIDA CON EL EMISOR ESP-NOW)
// ==========================================
typedef struct struct_message {
    float yaw, pitch, roll;
    int flex;
    float spo2;
    int heartRate;
} struct_message;

// Crear una variable para almacenar los datos entrantes
struct_message incomingData;

// ==========================================
// CALLBACK ESP-NOW (CUANDO LLEGAN DATOS)
// ==========================================
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataBytes, int len) {
    // 1. Verificar que el tamaño de los datos recibidos sea el esperado
    if (len != sizeof(incomingData)) {
      Serial.printf("Error: Tamaño de datos recibido (%d) no coincide con el esperado (%d)\n", len, sizeof(incomingData));
      return;
    }

    // 2. Copiar los datos recibidos a nuestra estructura
    memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));
    
    Serial.println("\n[ESP-NOW] Datos Recibidos. Reenviando por LoRa...");

    // 3. Construir el Frame LoRa (EXACTAMENTE IGUAL QUE EN Sender.ino)
    int idx = 0;
    bufferPaquete[idx++] = dir_destino;           // Byte 0: Destino final
    bufferPaquete[idx++] = dir_local;             // Byte 1: Quien reenvía (este nodo)
    bufferPaquete[idx++] = id_msjLoRa;            // Byte 2: ID del mensaje LoRa
    bufferPaquete[idx++] = sizeof(struct_message); // Byte 3: Tamaño del payload
    
    // 4. Copiar el payload (los datos de la estructura) al buffer
    memcpy(&bufferPaquete[idx], &incomingData, sizeof(struct_message));
    idx += sizeof(struct_message);
    
    // 5. Transmitir el paquete completo por LoRa
    LoRa.beginPacket();
    LoRa.write(bufferPaquete, idx);  // Enviar todo el frame
    LoRa.endPacket();

    // 6. Incrementar el ID para el próximo mensaje
    id_msjLoRa++;

    // 7. Mostrar información de depuración
    Serial.printf("LoRa Sent -> ID: %d | Dest: 0x%X\n", id_msjLoRa - 1, dir_destino);
    Serial.print("Datos: ");
    Serial.print(incomingData.yaw); Serial.print(", ");
    Serial.print(incomingData.pitch); Serial.print(", ");
    Serial.print(incomingData.roll); Serial.print(", ");
    Serial.print(incomingData.flex); Serial.print(", ");
    Serial.print(incomingData.spo2); Serial.print(", ");
    Serial.println(incomingData.heartRate);
    Serial.println("---------------------------------");
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    while (!Serial); // Esperar a que el puerto serie se conecte (útil para algunos ESP32)

    Serial.println("Iniciando Nodo Puente (ESP-NOW -> LoRa)...");

    // --- 1. Inicializar LoRa (basado en Sender.ino) ---
    SPI.begin(loraSCK, loraMISO, loraMOSI, loraNSS);
    LoRa.setPins(loraNSS, loraRST, loraDI0);
    
    if (!LoRa.begin(433E6)) { // Frecuencia: 433 MHz
        Serial.println("Fallo al iniciar LoRa!");
        while (1);
    }
    // Configurar parámetros para que coincidan con el receptor final
    LoRa.setSpreadingFactor(9);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(6);
    LoRa.setSyncWord(SyncWord);
    
    Serial.println("LoRa Inicializado correctamente en 433MHz.");
    Serial.print("Mi direccion LoRa: 0x"); Serial.println(dir_local, HEX);
    Serial.print("Reenviando a: 0x"); Serial.println(dir_destino, HEX);

    // --- 2. Inicializar ESP-NOW ---
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }
    
    // Registrar el callback para cuando se reciban datos por ESP-NOW
    esp_now_register_recv_cb(OnDataRecv);
    
    Serial.println("ESP-NOW inicializado y escuchando...");
    Serial.println("Esperando datos de emisores...");
    Serial.println("=================================");
}

// ==========================================
// LOOP
// ==========================================
void loop() {
    // No es necesario hacer nada aquí.
    // Todo ocurre en la función de callback OnDataRecv
    // cuando llega un paquete ESP-NOW.
    delay(10); // Pequeña pausa para no saturar el CPU
}