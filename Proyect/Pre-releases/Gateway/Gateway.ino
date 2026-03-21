// mosquitto_sub -h oc051111.ala.us-east-1.emqxsl.com -p 8883 -t test01 -u "edpi" -P "edpi" --cafile "C:\Users\edwin\OneDrive\Documentos\para-repo\Intelligent-IMU-Wireless-Nodes-ESP32-for-Posture-Reeducation-and-Strengthening\Proyect\Pre-releases\mocks\mqtt\emqxsl-ca.crt"
// Falta configuración de Bluethoo
// ESP32 38 PINS LILYGO - GATEWAY LoRa a MQTT sobre RED CELULAR
// Integración: Recibe datos por LoRa y los reenvía por MQTT vía LTE/NB-IoT

// ==========================================
// CONFIGURACIÓN LORA (RECEPTOR)
// ==========================================
#include <LoRa.h>

// pines RECEPTOR LORA
const int loraRST = 21;
const int loraDI0 = 22;
const int loraNSS = 5;
const int loraMOSI = 23;
const int loraMISO = 19;
const int loraSCK = 18;

int SyncWord = 0x22;

// ==========================================
// PROTOCOLO DE FRAMES (Igual que HeltecV3)
// ==========================================
byte dir_local = 0xB2; // MI DIRECCIÓN (Debe coincidir con destino del sender)

// Variables para "desempaquetar"
byte dir_envio     = 0; 
byte dir_remite    = 0; 
byte paqRcb_ID     = 0;
byte paqRcb_Estado = 0; 

// Estructura del payload (debe coincidir con el transmisor)
typedef struct struct_message {
    float roll_f, pitch_f, yaw_f;
    int ecg;
    float roll_a, pitch_a, yaw_a;
} struct_message;

// ==========================================
// CONFIGURACIÓN MÓDEM CELULAR (SIM7070)
// ==========================================
#define TINY_GSM_MODEM_SIM7070

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// --- CONFIGURACIÓN DE ACCESO A LA RED ---
const char apn[]      = "internet.itelcel.com"; 
const char gprsUser[] = ""; 
const char gprsPass[] = "";

// --- MQTT CON TLS/SSL ---
const char* broker = "oc051111.ala.us-east-1.emqxsl.com";
const char* topicPublish = "test01";
const int   port = 8883;
const char* mqtt_user = "edpi";
const char* mqtt_password = "edpi";

// --- CERTIFICADO CA ---
const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n" \
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n" \
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n" \
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n" \
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n" \
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n" \
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n" \
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n" \
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n" \
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n" \
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n" \
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n" \
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n" \
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n" \
"MrY=\n" \
"-----END CERTIFICATE-----\n";

// --- PINES MÓDEM ---
#define UART_BAUD   115200
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
#define LED_INDICATOR 12

HardwareSerial ModemSerial(1);

TinyGsm modem(ModemSerial);
TinyGsmClientSecure client(modem);
PubSubClient mqtt(client);

// Variables para control de envío MQTT
unsigned long ultimaReconexionMQTT = 0;
const unsigned long intervaloReconexionMQTT = 30000; // 30 segundos

// ==========================================
// FUNCIONES DEL MÓDEM CELULAR
// ==========================================
void encenderModem() {
  Serial.println("INICIO: Despertando el módulo celular...");
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(1000);
  digitalWrite(PWR_PIN, LOW);
  delay(2000);
}

bool inicializarModem() {
  Serial.println("PASO 1: Verificando comunicación AT...");
  if (!modem.restart()) {
    Serial.println("CRÍTICO: El módem no responde.");
    return false;
  }

  Serial.print("PASO 2: Buscando señal de torre celular...");
  if (!modem.waitForNetwork()) {
    Serial.println(" ERROR: Sin cobertura.");
    return false;
  }
  Serial.println(" OK");

  Serial.print("PASO 3: Activando sesión de datos...");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" ERROR APN.");
    return false;
  }

  Serial.print("\nIP: ");
  Serial.println(modem.getLocalIP());
  
  // Configurar cliente MQTT con TLS/SSL
  mqtt.setServer(broker, port);
  mqtt.setCallback(NULL); // No se reciben mensajes, solo se publica
  
  return true;
}

void reconectarMQTT() {
  if (millis() - ultimaReconexionMQTT < intervaloReconexionMQTT && !mqtt.connected()) {
    return; // Esperar antes de reintentar
  }
  
  ultimaReconexionMQTT = millis();
  
  Serial.print("Intentando acceso al Broker MQTT (TLS/SSL)...");
  String clienteID = "Gateway6B" + String(random(1000, 9999));
  
  // Configurar opciones SSL para SIM7070
  // Nota: SIM7070 soporta MQTT sobre TLS/SSL nativamente
  if (mqtt.connect(clienteID.c_str(), mqtt_user, mqtt_password)) {
    Serial.println(" CONECTADO.");
  } else {
    Serial.print(" FAILED (Code: ");
    Serial.print(mqtt.state());
    Serial.println("). Reintento en 30s...");
  }
}

bool enviarMQTT(struct_message &datos) {
  if (!mqtt.connected()) {
    reconectarMQTT();
    if (!mqtt.connected()) return false;
  }
  
  // -------- CREAR JSON --------
  String payload = "{";
  payload += "\"roll_f\":" + String(datos.roll_f, 2) + ",";
  payload += "\"pitch_f\":" + String(datos.pitch_f, 2) + ",";
  payload += "\"yaw_f\":" + String(datos.yaw_f, 2) + ",";
  payload += "\"ecg\":" + String(datos.ecg) + ",";
  payload += "\"roll_a\":" + String(datos.roll_a, 2) + ",";
  payload += "\"pitch_a\":" + String(datos.pitch_a, 2) + ",";
  payload += "\"yaw_a\":" + String(datos.yaw_a, 2);
  payload += "}";

  Serial.println(">>> Reenviando por MQTT (TLS/SSL):");
  Serial.println(payload);

  if (mqtt.publish(topicPublish, payload.c_str())) {
    Serial.println("MENSAJE RECIBIDO POR EL BROKER");
    return true;
  } else {
    Serial.println("Error de transmisión MQTT");
    return false;
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  Serial.println("=================================");
  Serial.println("GATEWAY LoRa -> MQTT sobre LTE");
  Serial.println("=================================");
  
  randomSeed(analogRead(0));
  
  // ----- INICIALIZAR LORA -----
  Serial.println("\n[LoRa] Inicializando...");
  SPI.begin(loraSCK, loraMISO, loraMOSI, loraNSS);
  LoRa.setPins(loraNSS, loraRST, loraDI0);

  if (!LoRa.begin(433E6)) {
    Serial.println("[LoRa] Fallo! Continuando solo con MQTT...");
  } else {
    LoRa.setSpreadingFactor(9);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(6);
    LoRa.setSyncWord(SyncWord);
    
    Serial.println("[LoRa] Listo Receptor");
    Serial.print("[LoRa] Mi direccion: 0x"); Serial.println(dir_local, HEX);
  }
  
  // ----- INICIALIZAR MÓDEM CELULAR -----
  Serial.println("\n[Celular] Inicializando...");
  encenderModem();
  ModemSerial.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(3000);
  
  if (!inicializarModem()) {
    Serial.println("[Celular] ADVERTENCIA: Modo solo LoRa (sin MQTT)");
  }
  
  Serial.println("\n=================================");
  Serial.println("Esperando frames LoRa...");
  Serial.println("=================================");
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================
void loop() {
  // Mantener conexión MQTT activa
  mqtt.loop();
  
  // Verificar recepción LoRa
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
      return; // No es para mí
    }
    
    if (len == sizeof(struct_message)) {
      
      struct_message cansatData;
      memcpy(&cansatData, &bufferPaquete[idx], len);
      
      // Mostrar datos recibidos por LoRa
      Serial.print("\n[LoRa] Recibido Frame VALIDO:");
      Serial.print("\n  De: 0x"); Serial.print(dir_remite, HEX);
      Serial.print("\n  Para: 0x"); Serial.print(dir_envio, HEX);
      Serial.print("\n  ID: "); Serial.print(paqRcb_ID);
      Serial.print("\n  roll_f = "); Serial.print(cansatData.roll_f);
      Serial.print("\n  pitch_f = "); Serial.print(cansatData.pitch_f);
      Serial.print("\n  yaw_f = "); Serial.print(cansatData.yaw_f);
      Serial.print("\n  ecg = "); Serial.print(cansatData.ecg);
      Serial.print("\n  roll_a = "); Serial.print(cansatData.roll_a);
      Serial.print("\n  pitch_a = "); Serial.print(cansatData.pitch_a);
      Serial.print("\n  yaw_a = "); Serial.print(cansatData.yaw_a);
      
      Serial.println();
      Serial.print("  RSSI ="); Serial.print(LoRa.packetRssi());
      Serial.print(" | SNR ="); Serial.println(LoRa.packetSnr(), 1);
      
      // REENVIAR POR MQTT
      Serial.println("\n[MQTT] Reenviando...");
      if (enviarMQTT(cansatData)) {
        Serial.println("[MQTT] Reenvío exitoso");
      } else {
        Serial.println("[MQTT] Error en reenvío");
      }
      
      Serial.println("---------------------------------");
      
    } else {
      Serial.println("Error: Tamaño de payload incorrecto");
    }
  }
  
  delay(10); // Pequeña pausa para estabilidad
}