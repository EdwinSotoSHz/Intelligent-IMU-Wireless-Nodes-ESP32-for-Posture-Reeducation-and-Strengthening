// ESP32 30 PINS (Conservar estos pines)

#include <WiFi.h>
#include <esp_now.h>
#include <LoRa.h>
#include <SPI.h>

// ==========================================
// CONFIGURACIÓN LORA (Pines del SX1278) - SIN CAMBIOS
// ==========================================
const int loraRST = 15;
const int loraDI0 = 2;
const int loraNSS = 5;
const int loraMOSI = 18;
const int loraMISO = 22;
const int loraSCK = 23;
int SyncWord = 0x22;

// PROTOCOLO DE FRAMES LORA - SIN CAMBIOS
byte dir_local   = 0xA1;
byte dir_destino = 0xB2;
byte id_msjLoRa = 0;
byte bufferPaquete[64];

// ==========================================
// ESTRUCTURA COMPLETA PARA LORA (PAYLOAD FUSIONADO)
// ==========================================
typedef struct struct_message_completo {
    float roll_f, pitch_f, yaw_f;  // Datos del antebrazo (Forearm)
    int ecg;                        // ECG (viene del brazo)
    float roll_a, pitch_a, yaw_a;   // Datos del brazo (Arm)
} struct_message_completo;

struct_message_completo datosFusionados;

// ==========================================
// ESTRUCTURAS PARA CADA NODO (RECEPCIÓN ESP-NOW)
// ==========================================
typedef struct struct_message_arm {
    float roll_a;
    float pitch_a;
    float yaw_a;
    int ecg;
    int node_id;
} struct_message_arm;

typedef struct struct_message_forearm {
    float roll_f;
    float pitch_f;
    float yaw_f;
    int node_id;
} struct_message_forearm;

// Variables para almacenar los últimos datos de cada nodo
struct_message_arm ultimosDatosBrazo;        // Nodo 1: roll_a, pitch_a, yaw_a, ecg
struct_message_forearm ultimosDatosAntebrazo; // Nodo 2: roll_f, pitch_f, yaw_f

// Flags para saber si hemos recibido datos de cada nodo ALGUNA VEZ
bool brazoInicializado = false;
bool antebrazoInicializado = false;

// Timestamps para saber cuándo recibimos cada dato
unsigned long tiempoUltimoBrazo = 0;
unsigned long tiempoUltimoAntebrazo = 0;
const unsigned long TIMEOUT_MS = 1000; // 1 segundo de timeout

// ==========================================
// CALLBACK ESP-NOW
// ==========================================
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataBytes, int len) {
    
    if (len == sizeof(struct_message_arm)) {
        struct_message_arm datosBrazo;
        memcpy(&datosBrazo, incomingDataBytes, sizeof(datosBrazo));
        
        if (datosBrazo.node_id == 1) {
            ultimosDatosBrazo = datosBrazo;
            tiempoUltimoBrazo = millis();
            
            if (!brazoInicializado) {
                brazoInicializado = true;
                Serial.println("BRAZO detectado por primera vez");
            }
            
            Serial.print("\n[ESP-NOW] BRAZO: ");
            Serial.print("roll_a="); Serial.print(datosBrazo.roll_a, 1);
            Serial.print(", pitch_a="); Serial.print(datosBrazo.pitch_a, 1);
            Serial.print(", yaw_a="); Serial.print(datosBrazo.yaw_a, 1);
            Serial.print(", ecg="); Serial.println(datosBrazo.ecg);
            
            enviarLoRaConDatosDisponibles();
        }
    }
    else if (len == sizeof(struct_message_forearm)) {
        struct_message_forearm datosAntebrazo;
        memcpy(&datosAntebrazo, incomingDataBytes, sizeof(datosAntebrazo));
        
        if (datosAntebrazo.node_id == 2) {
            ultimosDatosAntebrazo = datosAntebrazo;
            tiempoUltimoAntebrazo = millis();
            
            if (!antebrazoInicializado) {
                antebrazoInicializado = true;
                Serial.println("ANTEBRAZO detectado por primera vez");
            }
            
            Serial.print("\n[ESP-NOW] ANTEBRAZO: ");
            Serial.print("roll_f="); Serial.print(datosAntebrazo.roll_f, 1);
            Serial.print(", pitch_f="); Serial.print(datosAntebrazo.pitch_f, 1);
            Serial.print(", yaw_f="); Serial.println(datosAntebrazo.yaw_f, 1);
            
            enviarLoRaConDatosDisponibles();
        }
    }
    else {
        Serial.print("\n[ESP-NOW] Error: Tamaño desconocido: ");
        Serial.println(len);
        return;
    }
}

// ==========================================
// FUNCIÓN PARA ENVIAR LORA CON LOS DATOS DISPONIBLES
// ==========================================
void enviarLoRaConDatosDisponibles() {
    unsigned long ahora = millis();
    
    bool brazoActivo = brazoInicializado && (ahora - tiempoUltimoBrazo) < TIMEOUT_MS;
    bool antebrazoActivo = antebrazoInicializado && (ahora - tiempoUltimoAntebrazo) < TIMEOUT_MS;
    
    if (!brazoActivo && !antebrazoActivo) {
        return;
    }
    
    const float FLOAT_NO_DATA = 0.0;
    const int INT_NO_DATA = 0;
    
    // Llenar datos del antebrazo (Forearm) - vienen del nodo 2
    if (antebrazoActivo) {
        datosFusionados.roll_f = ultimosDatosAntebrazo.roll_f;
        datosFusionados.pitch_f = ultimosDatosAntebrazo.pitch_f;
        datosFusionados.yaw_f = ultimosDatosAntebrazo.yaw_f;
    } else {
        datosFusionados.roll_f = FLOAT_NO_DATA;
        datosFusionados.pitch_f = FLOAT_NO_DATA;
        datosFusionados.yaw_f = FLOAT_NO_DATA;
    }
    
    // Llenar datos del brazo (Arm) - vienen del nodo 1
    if (brazoActivo) {
        datosFusionados.roll_a = ultimosDatosBrazo.roll_a;
        datosFusionados.pitch_a = ultimosDatosBrazo.pitch_a;
        datosFusionados.yaw_a = ultimosDatosBrazo.yaw_a;
        datosFusionados.ecg = ultimosDatosBrazo.ecg;  // El ECG viene en el nodo del brazo
    } else {
        datosFusionados.roll_a = FLOAT_NO_DATA;
        datosFusionados.pitch_a = FLOAT_NO_DATA;
        datosFusionados.yaw_a = FLOAT_NO_DATA;
        datosFusionados.ecg = INT_NO_DATA;
    }
    
    int idx = 0;
    bufferPaquete[idx++] = dir_destino;
    bufferPaquete[idx++] = dir_local;
    bufferPaquete[idx++] = id_msjLoRa;
    bufferPaquete[idx++] = sizeof(struct_message_completo);
    
    memcpy(&bufferPaquete[idx], &datosFusionados, sizeof(struct_message_completo));
    idx += sizeof(struct_message_completo);
    
    LoRa.beginPacket();
    LoRa.write(bufferPaquete, idx);
    LoRa.endPacket();
    
    Serial.println("\n[LoRa] Datos fusionados enviados:");
    
    if (antebrazoActivo) {
        Serial.print("  ANTEBRAZO: roll_f="); Serial.print(datosFusionados.roll_f, 1);
        Serial.print(", pitch_f="); Serial.print(datosFusionados.pitch_f, 1);
        Serial.print(", yaw_f="); Serial.println(datosFusionados.yaw_f, 1);
    } else {
        Serial.println("  ANTEBRAZO: NO DISPONIBLE");
    }
    
    if (brazoActivo) {
        Serial.print("  BRAZO: roll_a="); Serial.print(datosFusionados.roll_a, 1);
        Serial.print(", pitch_a="); Serial.print(datosFusionados.pitch_a, 1);
        Serial.print(", yaw_a="); Serial.print(datosFusionados.yaw_a, 1);
        Serial.print(", ecg="); Serial.println(datosFusionados.ecg);
    } else {
        Serial.println("  BRAZO: NO DISPONIBLE");
    }
    
    Serial.print("  ID LoRa: "); Serial.print(id_msjLoRa);
    Serial.print(" | Destino: 0x"); Serial.println(dir_destino, HEX);
    Serial.println("---------------------------------");
    
    id_msjLoRa++;
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Iniciando Nodo Puente (ESP-NOW -> LoRa)");
    Serial.println("Modo: 1 o 2 nodos");

    SPI.begin(loraSCK, loraMISO, loraMOSI, loraNSS);
    LoRa.setPins(loraNSS, loraRST, loraDI0);
    
    if (!LoRa.begin(433E6)) {
        Serial.println("Fallo al iniciar LoRa!");
        while (1);
    }
    LoRa.setSpreadingFactor(9);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(6);
    LoRa.setSyncWord(SyncWord);
    
    Serial.println("LoRa Inicializado");
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }
    
    esp_now_register_recv_cb(OnDataRecv);
    
    brazoInicializado = false;
    antebrazoInicializado = false;
    
    Serial.println("ESP-NOW inicializado - Esperando datos...");
    Serial.println("=================================");
}

// ==========================================
// LOOP
// ==========================================
void loop() {
    static unsigned long ultimaVerificacion = 0;
    unsigned long ahora = millis();
    
    if (ahora - ultimaVerificacion > 1000) {
        ultimaVerificacion = ahora;
        
        bool brazoActivo = brazoInicializado && (ahora - tiempoUltimoBrazo) < TIMEOUT_MS;
        bool antebrazoActivo = antebrazoInicializado && (ahora - tiempoUltimoAntebrazo) < TIMEOUT_MS;
        
        static bool brazoEstabaActivo = false;
        static bool antebrazoEstabaActivo = false;
        
        if (brazoActivo != brazoEstabaActivo) {
            brazoEstabaActivo = brazoActivo;
            Serial.print("[ESTADO] BRAZO: ");
            Serial.println(brazoActivo ? "CONECTADO" : "DESCONECTADO");
        }
        
        if (antebrazoActivo != antebrazoEstabaActivo) {
            antebrazoEstabaActivo = antebrazoActivo;
            Serial.print("[ESTADO] ANTEBRAZO: ");
            Serial.println(antebrazoActivo ? "CONECTADO" : "DESCONECTADO");
        }
    }
    
    delay(10);
}