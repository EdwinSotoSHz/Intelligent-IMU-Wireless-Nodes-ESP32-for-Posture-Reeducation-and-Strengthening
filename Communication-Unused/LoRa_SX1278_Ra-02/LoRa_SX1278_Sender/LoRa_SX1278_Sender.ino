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

// Mejor estructura (Payload)
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

  Serial.println("Listo (formato: int,int,float,float,text)");
  Serial.print("Mi direccion: 0x"); Serial.println(dir_local, HEX);
  Serial.print("Envio a: 0x"); Serial.println(dir_destino, HEX);
  Serial.println("---------------------------------");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Validacion para test (cambiar por expresion regular)
    int comma1 = input.indexOf(',');
    int comma2 = input.indexOf(',', comma1 + 1);
    int comma3 = input.indexOf(',', comma2 + 1);
    int comma4 = input.indexOf(',', comma3 + 1);
    if (comma1 == -1 || comma2 == -1 || comma3 == -1 || comma4 == -1) {
      Serial.println("Formato invalido (,)");
      return;
    }

    //Usar nueva estructura numeros y char
    Payload cansatData;
    cansatData.num1 = input.substring(0, comma1).toInt();
    cansatData.num2 = input.substring(comma1 + 1, comma2).toInt();
    cansatData.num3 = input.substring(comma2 + 1, comma3).toFloat();
    cansatData.num4 = input.substring(comma3 + 1, comma4).toFloat();    
    String textStr = input.substring(comma4 + 1);
    textStr.toCharArray(cansatData.text, 6);
    for (int i = textStr.length(); i < 5; i++) { // Rellenar con espacios si no cumple con 5 caracteres 
      cansatData.text[i] = ' ';
    }

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
    bufferPaquete[idx++] = sizeof(Payload);        // Byte 3: Tamaño del payload (fijo: 14 bytes)
    
    // 2. PAYLOAD (ESTRUCTURA BINARIA)
    memcpy(&bufferPaquete[idx], &cansatData, sizeof(Payload));
    idx += sizeof(Payload);
    
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
    Serial.print("\n  Tamano Payload: "); Serial.print(sizeof(Payload));
    Serial.print("\n  num1 = "); Serial.print(cansatData.num1);
    Serial.print("\n  num2 = "); Serial.print(cansatData.num2);
    Serial.print("\n  num3 = "); Serial.print(cansatData.num3);
    Serial.print("\n  num4 = "); Serial.print(cansatData.num4);
    Serial.print("\n  char[5] = "); Serial.print(cansatData.text);

    Serial.println();
    Serial.print("(");
    Serial.print(millis() - t0);
    Serial.println(" ms)");
    Serial.println("---------------------------------");
    Serial.println("");

  }
  delay(15);
}

// 1,222,3.3,4.4,qwert