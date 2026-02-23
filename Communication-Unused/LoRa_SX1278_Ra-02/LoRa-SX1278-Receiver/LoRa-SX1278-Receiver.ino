#include <LoRa.h>

// pines RECEPTOR
const int loraRST = 15;
const int loraDI0 = 2;
const int loraNSS = 5;
const int loraMOSI = 18;
const int loraMISO = 22;
const int loraSCK = 23;

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

// Mejor estructura
struct Payload {
  int16_t num1;  // 2 bytes (-999 a 999)
  int16_t num2; // 2 bytes (-999 a 999)
  float num3; // 4 bytes
  float num4;  // 4 bytes
  char text[6]; // 5 caracteres max
};

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
  
  // Mínimo: cabecera (4 bytes) + algo de payload
  if (packetSize >= 4) {

    // Buffer para recibir todo el paquete
    byte bufferPaquete[packetSize];
    LoRa.readBytes(bufferPaquete, packetSize);
    
    // ==========================================
    // PROCESAR FRAME (DESENCAPSULAR)
    // ==========================================
    int idx = 0;
    
    // 1. LEER CABECERA
    dir_envio  = bufferPaquete[idx++]; 
    dir_remite = bufferPaquete[idx++]; 
    paqRcb_ID  = bufferPaquete[idx++]; 
    byte len   = bufferPaquete[idx++];  // Tamaño del payload
    
    // 2. VERIFICACIONES DE INTEGRIDAD
    // ¿El tamaño coincide?
    if (packetSize - 4 != len) {
      Serial.println("Error: Tamaño de paquete no coincide");
      return;
    }
    
    // ¿Es para mí o es broadcast (0xFF)?
    if (dir_envio != dir_local && dir_envio != 0xFF) {
      // Mensaje no es para mí - lo ignoramos silenciosamente
      return;
    }
    
    // 3. SOLO SI ES PARA MÍ, PROCESO EL PAYLOAD
    if (len == sizeof(Payload)) {
      
      // Extraer la estructura del payload
      Payload cansatData;
      memcpy(&cansatData, &bufferPaquete[idx], len);
      
      // Imprimir
      Serial.print("Recibido Frame VALIDO:");
      Serial.print("\n  De: 0x"); Serial.print(dir_remite, HEX);
      Serial.print("\n  Para: 0x"); Serial.print(dir_envio, HEX);
      Serial.print("\n  ID: "); Serial.print(paqRcb_ID);
      Serial.print("\n  num1 = "); Serial.print(cansatData.num1);
      Serial.print("\n  num2 = "); Serial.print(cansatData.num2);
      Serial.print("\n  num3 = "); Serial.print(cansatData.num3);
      Serial.print("\n  num4 = "); Serial.print(cansatData.num4);
      Serial.print("\n  char[5] = "); Serial.print(cansatData.text);
      
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