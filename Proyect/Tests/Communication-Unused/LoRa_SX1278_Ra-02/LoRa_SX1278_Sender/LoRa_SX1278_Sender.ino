#include <LoRa.h>

// pines TRANSMISOR
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
// Byte 0: Destino
// Byte 1: Remitente  
// Byte 2: ID Mensaje
// Byte 3: Largo del Payload
// Byte 4...N: Payload (estructura binaria)
// ==========================================
byte dir_local   = 0xA1; // MI DIRECCIÓN (Cambiada para no chocar)
byte dir_destino = 0xB2; // DIRECCIÓN DEL RECEPTOR (Cambiada)

byte id_msjLoRa = 0;     // Número de secuencia

// Buffer para construir el paquete completo
byte bufferPaquete[64];  // Suficiente para nuestra estructura

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

  Serial.println("Listo (formato: yaw,pitch,roll,flex,spo2,heartRate)");
  Serial.print("Mi direccion: 0x"); Serial.println(dir_local, HEX);
  Serial.print("Envio a: 0x"); Serial.println(dir_destino, HEX);
  Serial.println("---------------------------------");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Validacion para test
    int comma1 = input.indexOf(',');
    int comma2 = input.indexOf(',', comma1 + 1);
    int comma3 = input.indexOf(',', comma2 + 1);
    int comma4 = input.indexOf(',', comma3 + 1);
    int comma5 = input.indexOf(',', comma4 + 1);
    if (comma1 == -1 || comma2 == -1 || comma3 == -1 || comma4 == -1 || comma5 == -1) {
      Serial.println("Formato invalido (necesita 5 comas)");
      return;
    }

    // Usar nueva estructura
    struct_message cansatData;
    cansatData.yaw   = input.substring(0, comma1).toFloat();
    cansatData.pitch = input.substring(comma1 + 1, comma2).toFloat();
    cansatData.roll  = input.substring(comma2 + 1, comma3).toFloat();
    cansatData.flex  = input.substring(comma3 + 1, comma4).toInt();
    cansatData.spo2  = input.substring(comma4 + 1, comma5).toFloat();
    cansatData.heartRate = input.substring(comma5 + 1).toInt();

    // tiempo transcurrido
    unsigned long t0 = millis();

    // ==========================================
    // CONSTRUIR FRAME (IGUAL QUE HELTEC)
    // ==========================================
    int idx = 0;
    
    // 1. CABECERA (METADATOS)
    bufferPaquete[idx++] = dir_destino;           // Byte 0: ¿Para quién es?
    bufferPaquete[idx++] = dir_local;              // Byte 1: ¿Quién soy yo?
    bufferPaquete[idx++] = id_msjLoRa;             // Byte 2: Folio del mensaje
    bufferPaquete[idx++] = sizeof(struct_message); // Byte 3: Tamaño del payload
    
    // 2. PAYLOAD (ESTRUCTURA BINARIA)
    memcpy(&bufferPaquete[idx], &cansatData, sizeof(struct_message));
    idx += sizeof(struct_message);
    
    // 3. ENVIAR POR LORA
    LoRa.beginPacket();
    LoRa.write(bufferPaquete, idx);  // Enviar todo el frame
    LoRa.endPacket();

    // Incrementar ID para el próximo mensaje
    id_msjLoRa++;

    // Imprimir
    Serial.print("Enviado Frame:");
    Serial.print("\n  Destino: 0x"); Serial.print(dir_destino, HEX);
    Serial.print("\n  Remite: 0x"); Serial.print(dir_local, HEX);
    Serial.print("\n  ID: "); Serial.print(id_msjLoRa - 1);
    Serial.print("\n  Tamano Payload: "); Serial.print(sizeof(struct_message));
    Serial.print("\n  yaw = "); Serial.print(cansatData.yaw);
    Serial.print("\n  pitch = "); Serial.print(cansatData.pitch);
    Serial.print("\n  roll = "); Serial.print(cansatData.roll);
    Serial.print("\n  flex = "); Serial.print(cansatData.flex);
    Serial.print("\n  spo2 = "); Serial.print(cansatData.spo2);
    Serial.print("\n  heartRate = "); Serial.print(cansatData.heartRate);

    Serial.println();
    Serial.print("(");
    Serial.print(millis() - t0);
    Serial.println(" ms)");
    Serial.println("---------------------------------");
    Serial.println("");

  }
  delay(15);
}

// Ejemplo: 1.5,2.3,3.7,100,98.6,75