// mosquitto_sub -h oc051111.ala.us-east-1.emqxsl.com -p 8883 -t test01 -u "edpi" -P "edpi" --cafile "C:\Users\edwin\OneDrive\Documentos\para-repo\Intelligent-IMU-Wireless-Nodes-ESP32-for-Posture-Reeducation-and-Strengthening\Proyect\Pre-releases\mocks\mqtt\emqxsl-ca.crt"
// ESP32 38 PINS LILYGO - GATEWAY LoRa a MQTT sobre RED CELULAR con fallback WiFi
// Integración: Recibe datos por LoRa y los reenvía por MQTT vía LTE/NB-IoT o WiFi

// ==========================================
// CONFIGURACIÓN LORA (RECEPTOR)
// ==========================================
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>  // ← LIBRERÍA AGREGADA PARA TLS EN WiFi

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

// --- CONFIGURACIÓN WiFi (FALLBACK) ---
const char* ssid_wifi = "AsusE";
const char* password_wifi = "23011edpi";

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

// Variables para control de conexión
enum ConnectionType { CONN_NONE, CONN_LTE, CONN_WIFI };
ConnectionType currentConnection = CONN_NONE;
bool mqttConnected = false;

// Objeto para cliente WiFi secure (se declara global para que persista)
WiFiClientSecure wifiClientSecure;

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

  Serial.print("\nIP LTE: ");
  Serial.println(modem.getLocalIP());
  
  return true;
}

// ==========================================
// FUNCIONES DE CONEXIÓN WiFi
// ==========================================
bool conectarWiFi() {
  Serial.println("\n[WiFi] Intentando conectar a: " + String(ssid_wifi));
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_wifi, password_wifi);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Conectado exitosamente!");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\n[WiFi] Falló la conexión");
    return false;
  }
}

// ==========================================
// FUNCIONES MQTT (COMUNES)
// ==========================================
void configurarClienteMQTT() {
  if (currentConnection == CONN_WIFI) {
    // Para WiFi, usar cliente WiFi estándar con TLS
    wifiClientSecure.setCACert(ca_cert);
    mqtt.setClient(wifiClientSecure);
  }
  // Para LTE, el cliente ya está configurado como TinyGsmClientSecure
  
  mqtt.setServer(broker, port);
  mqtt.setCallback(NULL);
}

void reconectarMQTT() {
  if (millis() - ultimaReconexionMQTT < intervaloReconexionMQTT && !mqtt.connected()) {
    return;
  }
  
  ultimaReconexionMQTT = millis();
  
  Serial.print("Intentando acceso al Broker MQTT (TLS/SSL) vía ");
  Serial.print(currentConnection == CONN_LTE ? "LTE" : "WiFi");
  Serial.print("...");
  
  String clienteID = "Gateway6B" + String(random(1000, 9999));
  
  if (mqtt.connect(clienteID.c_str(), mqtt_user, mqtt_password)) {
    Serial.println(" CONECTADO.");
    mqttConnected = true;
  } else {
    Serial.print(" FAILED (Code: ");
    Serial.print(mqtt.state());
    Serial.println(")");
    mqttConnected = false;
  }
}

// ==========================================
// FUNCIÓN PRINCIPAL DE CONEXIÓN A INTERNET
// ==========================================
bool conectarInternet() {
  // Intentar primero LTE
  Serial.println("\n[INFO] Intentando conexión vía LTE...");
  encenderModem();
  ModemSerial.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(3000);
  
  if (inicializarModem()) {
    currentConnection = CONN_LTE;
    configurarClienteMQTT();
    Serial.println("[INFO] Conexión establecida vía LTE");
    return true;
  }
  
  // Si falla LTE, intentar WiFi
  Serial.println("\n[INFO] LTE falló, intentando WiFi como respaldo...");
  if (conectarWiFi()) {
    currentConnection = CONN_WIFI;
    configurarClienteMQTT();
    Serial.println("[INFO] Conexión establecida vía WiFi");
    return true;
  }
  
  Serial.println("[ERROR] No hay conexión a internet disponible");
  currentConnection = CONN_NONE;
  return false;
}

// ==========================================
// ENVÍO MQTT
// ==========================================
bool enviarMQTT(struct_message &datos) {
  if (currentConnection == CONN_NONE) {
    Serial.println("[MQTT] Sin conexión a internet, reintentando...");
    if (!conectarInternet()) {
      return false;
    }
  }
  
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

  Serial.println(">>> Reenviando por MQTT (TLS/SSL) vía " + String(currentConnection == CONN_LTE ? "LTE" : "WiFi") + ":");
  Serial.println(payload);

  if (mqtt.publish(topicPublish, payload.c_str())) {
    Serial.println("MENSAJE RECIBIDO POR EL BROKER");
    return true;
  } else {
    Serial.println("Error de transmisión MQTT");
    mqttConnected = false;
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
  Serial.println("GATEWAY LoRa -> MQTT (LTE/WiFi)");
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
  
  // ----- CONEXIÓN A INTERNET (LTE con fallback WiFi) -----
  Serial.println("\n[Internet] Estableciendo conexión...");
  if (!conectarInternet()) {
    Serial.println("[Internet] ADVERTENCIA: Modo solo LoRa (sin MQTT)");
  }
  
  Serial.println("\n=================================");
  Serial.println("Esperando frames LoRa...");
  Serial.println("=================================");
}

// ==========================================
// LOOP PRINCIPAL
// ==========================================
void loop() {
  // Mantener conexión MQTT activa si hay internet
  if (currentConnection != CONN_NONE) {
    mqtt.loop();
  } else {
    // Si no hay conexión, intentar reconectar periódicamente
    static unsigned long ultimoIntentoConexion = 0;
    if (millis() - ultimoIntentoConexion > 60000) { // Cada 60 segundos
      ultimoIntentoConexion = millis();
      conectarInternet();
    }
  }
  
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
      
      // REENVIAR POR MQTT (solo si hay conexión a internet)
      if (currentConnection != CONN_NONE) {
        Serial.println("\n[MQTT] Reenviando...");
        if (enviarMQTT(cansatData)) {
          Serial.println("[MQTT] Reenvío exitoso");
        } else {
          Serial.println("[MQTT] Error en reenvío");
        }
      } else {
        Serial.println("\n[ERROR] Sin conexión a internet, datos no enviados");
      }
      
      Serial.println("---------------------------------");
      
    } else {
      Serial.println("Error: Tamaño de payload incorrecto");
    }
  }
  
  delay(10); // Pequeña pausa para estabilidad
}