// ESP32 38 PINS (Modificarlos a los de la liligo)

#include <LoRa.h>

// pines RECEPTOR
const int loraRST = 23;
const int loraDI0 = 4;
const int loraNSS = 15;
const int loraMOSI = 32;
const int loraMISO = 35;
const int loraSCK = 33;

int SyncWord = 0x22;

// ==========================================
// PROTOCOLO DE FRAMES (Igual que HeltecV3)
// ==========================================
byte dir_local = 0xB2; // MI DIRECCIÓN (Debe coincidir con destino del sender)

// Variables para "desempaquetar"
byte dir_envio     = 0; // ¿Para quién dice la carta que es?
byte dir_remite    = 0; // ¿Quién la firmó?
byte paqRcb_ID     = 0;
byte paqRcb_Estado = 0; // Semáforo: 1=Aceptado, 2=Corrupto, 3=No es para mí

// Nueva estructura del payload
typedef struct struct_message {
    float yaw, pitch, roll;
    int flex;
    float spo2;
    int heartRate;
} struct_message;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // SPI
  SPI.begin(loraSCK, loraMISO, loraMOSI, loraNSS);
  LoRa.setPins(loraNSS, loraRST, loraDI0);

  // 433mhz
  if (!LoRa.begin(433E6)) {
    Serial.println("Fallo LoRa!");
    while (1);
  }

  // Configurar parámetros con mejor optimización (aun confiable, pero solo para test)
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(6);
  LoRa.setSyncWord(SyncWord);

  Serial.println("Listo Receptor");
  Serial.print("Mi direccion: 0x"); Serial.println(dir_local, HEX);
  Serial.println("Esperando frames...");
  Serial.println("---------------------------------");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  
  if (packetSize >= 4) {

    byte bufferPaquete[packetSize];
    LoRa.readBytes(bufferPaquete, packetSize);
    
    int idx = 0;
    
    dir_envio  = bufferPaquete[idx++]; 
    dir_remite = bufferPaquete[idx++]; 
    paqRcb_ID  = bufferPaquete[idx++]; 
    byte len   = bufferPaquete[idx++];
    
    if (packetSize - 4 != len) {
      Serial.println("Error: Tamaño de paquete no coincide");
      return;
    }
    
    if (dir_envio != dir_local && dir_envio != 0xFF) {
      return;
    }
    
    if (len == sizeof(struct_message)) {
      
      struct_message cansatData;
      memcpy(&cansatData, &bufferPaquete[idx], len);
      
      Serial.print("Recibido Frame VALIDO:");
      Serial.print("\n  De: 0x"); Serial.print(dir_remite, HEX);
      Serial.print("\n  Para: 0x"); Serial.print(dir_envio, HEX);
      Serial.print("\n  ID: "); Serial.print(paqRcb_ID);
      Serial.print("\n  yaw = "); Serial.print(cansatData.yaw);
      Serial.print("\n  pitch = "); Serial.print(cansatData.pitch);
      Serial.print("\n  roll = "); Serial.print(cansatData.roll);
      Serial.print("\n  flex = "); Serial.print(cansatData.flex);
      Serial.print("\n  spo2 = "); Serial.print(cansatData.spo2);
      Serial.print("\n  heartRate = "); Serial.print(cansatData.heartRate);
      
      Serial.println();
      Serial.print("RSSI ="); Serial.print(LoRa.packetRssi());
      Serial.print(" | SNR ="); Serial.println(LoRa.packetSnr(), 1);
      Serial.println("---------------------------------");
      Serial.println("");
      
    } else {
      Serial.println("Error: Tamaño de payload incorrecto");
    }
  }
  delay(10);
}