// ESP32 Nodo Maestro - ESP-NOW a MQTT
#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ==========================================
// CONFIGURACIÓN WiFi
// ==========================================
const char* ssid = "AsusE";
const char* wifi_password = "23011edpi";

// ==========================================
// CONFIGURACIÓN MQTT
// ==========================================
const char* mqtt_server = "oc051111.ala.us-east-1.emqxsl.com";
const int mqtt_port = 8883;
const char* mqtt_user = "edpi";
const char* mqtt_password = "edpi";
const char* mqtt_topic = "test01";

// Certificado CA - FORMATO CORREGIDO
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
struct_message_arm ultimosDatosBrazo;
struct_message_forearm ultimosDatosAntebrazo;

// Flags para saber si hemos recibido datos de cada nodo
bool brazoInicializado = false;
bool antebrazoInicializado = false;

// Timestamps para saber cuándo recibimos cada dato
unsigned long tiempoUltimoBrazo = 0;
unsigned long tiempoUltimoAntebrazo = 0;
const unsigned long TIMEOUT_MS = 1000; // 1 segundo de timeout

// ==========================================
// VARIABLES MQTT
// ==========================================
WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMsgTime = 0;
const long mqtt_interval = 100; // Enviar cada 100ms cuando hay datos nuevos
StaticJsonDocument<256> jsonDoc;

// ==========================================
// FUNCIÓN PARA ENVIAR DATOS POR MQTT
// ==========================================
void enviarMQTT() {
    unsigned long ahora = millis();
    
    bool brazoActivo = brazoInicializado && (ahora - tiempoUltimoBrazo) < TIMEOUT_MS;
    bool antebrazoActivo = antebrazoInicializado && (ahora - tiempoUltimoAntebrazo) < TIMEOUT_MS;
    
    // Solo enviar si al menos un nodo está activo
    if (!brazoActivo && !antebrazoActivo) {
        return;
    }
    
    // Limpiar el documento JSON
    jsonDoc.clear();
    
    // Agregar datos del antebrazo si están disponibles
    if (antebrazoActivo) {
        jsonDoc["roll_f"] = ultimosDatosAntebrazo.roll_f;
        jsonDoc["pitch_f"] = ultimosDatosAntebrazo.pitch_f;
        jsonDoc["yaw_f"] = ultimosDatosAntebrazo.yaw_f;
    }
    
    // Agregar datos del brazo si están disponibles
    if (brazoActivo) {
        jsonDoc["roll_a"] = ultimosDatosBrazo.roll_a;
        jsonDoc["pitch_a"] = ultimosDatosBrazo.pitch_a;
        jsonDoc["yaw_a"] = ultimosDatosBrazo.yaw_a;
        jsonDoc["ecg"] = ultimosDatosBrazo.ecg;
    }
    
    // Convertir a string JSON
    char jsonString[256];
    serializeJson(jsonDoc, jsonString);
    
    // Publicar en MQTT
    if (client.publish(mqtt_topic, jsonString)) {
        Serial.print("[MQTT] Enviado: ");
        Serial.println(jsonString);
    } else {
        Serial.print("[MQTT] Error al publicar - Estado MQTT: ");
        Serial.println(client.state());
    }
}

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
                Serial.println("\n[ESP-NOW] BRAZO detectado por primera vez");
            }
            
            Serial.print("\n[ESP-NOW] BRAZO: ");
            Serial.print("roll_a="); Serial.print(datosBrazo.roll_a, 1);
            Serial.print(", pitch_a="); Serial.print(datosBrazo.pitch_a, 1);
            Serial.print(", yaw_a="); Serial.print(datosBrazo.yaw_a, 1);
            Serial.print(", ecg="); Serial.println(datosBrazo.ecg);
            
            enviarMQTT();
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
                Serial.println("\n[ESP-NOW] ANTEBRAZO detectado por primera vez");
            }
            
            Serial.print("\n[ESP-NOW] ANTEBRAZO: ");
            Serial.print("roll_f="); Serial.print(datosAntebrazo.roll_f, 1);
            Serial.print(", pitch_f="); Serial.print(datosAntebrazo.pitch_f, 1);
            Serial.print(", yaw_f="); Serial.println(datosAntebrazo.yaw_f, 1);
            
            enviarMQTT();
        }
    }
    else {
        Serial.print("\n[ESP-NOW] Error: Tamaño desconocido: ");
        Serial.println(len);
    }
}

// ==========================================
// CONFIGURACIÓN WIFI
// ==========================================
void setup_wifi() {
    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
}

// ==========================================
// RECONEXIÓN MQTT
// ==========================================
void reconnect() {
    int intentos = 0;
    while (!client.connected() && intentos < 5) {
        intentos++;
        Serial.print("Conectando MQTT (intento ");
        Serial.print(intentos);
        Serial.print("/5)...");
        
        String clientId = "ESP32Master-" + String(random(0xffff), HEX);
        
        // Configurar timeout más largo
        espClient.setTimeout(30000);
        
        if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
            Serial.println("Conectado!");
            Serial.print("Cliente ID: ");
            Serial.println(clientId);
        } else {
            Serial.print("Error, rc=");
            Serial.print(client.state());
            Serial.print(" - ");
            
            // Mostrar significado del error
            switch(client.state()) {
                case -4:
                    Serial.println("Timeout de conexión");
                    break;
                case -3:
                    Serial.println("Conexión perdida");
                    break;
                case -2:
                    Serial.println("Error de red");
                    break;
                case -1:
                    Serial.println("Conexión fallida - ¿Certificado incorrecto?");
                    break;
                case 1:
                    Serial.println("Versión de protocolo incorrecta");
                    break;
                case 2:
                    Serial.println("ID de cliente rechazado");
                    break;
                case 3:
                    Serial.println("Servidor no disponible");
                    break;
                case 4:
                    Serial.println("Usuario/contraseña incorrectos");
                    break;
                case 5:
                    Serial.println("No autorizado");
                    break;
                default:
                    Serial.println("Error desconocido");
            }
            
            Serial.println("Reintentando en 3s...");
            delay(3000);
        }
    }
    
    if (!client.connected()) {
        Serial.println("No se pudo conectar MQTT después de 5 intentos");
    }
}

// ==========================================
// VERIFICAR CONEXIÓN MQTT
// ==========================================
void verificarConexionMQTT() {
    static unsigned long ultimaVerificacionMQTT = 0;
    unsigned long ahora = millis();
    
    // Verificar cada 10 segundos
    if (ahora - ultimaVerificacionMQTT > 10000) {
        ultimaVerificacionMQTT = ahora;
        
        if (!client.connected()) {
            Serial.println("MQTT desconectado, reconectando...");
            reconnect();
        } else {
            // Verificar que la conexión siga viva con un ping
            if (!client.loop()) {
                Serial.println("MQTT loop falló, reconectando...");
                reconnect();
            }
        }
    }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    Serial.println("\n=================================");
    Serial.println("Iniciando Nodo Maestro ESP-NOW -> MQTT");
    Serial.println("=================================");

    // Configurar WiFi
    setup_wifi();

    // Configurar MQTT
    espClient.setCACert(ca_cert);
    espClient.setTimeout(30000); // Timeout más largo
    client.setServer(mqtt_server, mqtt_port);
    client.setKeepAlive(60); // Keep alive de 60 segundos
    
    // Intentar conexión MQTT inicial
    reconnect();
    
    // Configurar ESP-NOW
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }
    
    esp_now_register_recv_cb(OnDataRecv);
    
    // Inicializar flags
    brazoInicializado = false;
    antebrazoInicializado = false;
    
    Serial.println("ESP-NOW inicializado - Esperando datos...");
    Serial.println("=================================\n");
}

// ==========================================
// LOOP
// ==========================================
void loop() {
    // Verificar y mantener conexión MQTT
    verificarConexionMQTT();
    
    if (client.connected()) {
        client.loop();
    }
    
    // Verificar estado de los nodos cada segundo
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
        
        // Mostrar estado MQTT periódicamente
        if (client.connected()) {
            Serial.print("[MQTT] Conectado - Estado: ");
            Serial.println(client.state());
        }
    }
    
    delay(10);
}