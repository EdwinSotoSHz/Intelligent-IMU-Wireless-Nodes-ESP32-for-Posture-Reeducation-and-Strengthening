#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// ==========================================
// CONFIGURACIÓN WIFI
// ==========================================
const char* ssid = "AsusE";
const char* wifi_password = "23011edpi";

// ==========================================
// CONFIGURACIÓN MQTT CON TLS/SSL
// ==========================================
const char* broker = "oc051111.ala.us-east-1.emqxsl.com";
const char* topicPublish = "test01";
const int port = 8883;
const char* mqtt_user = "edpi";
const char* mqtt_password = "edpi";

// CERTIFICADO CA
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
// ESTRUCTURAS PARA CADA NODO
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

// Buffer para almacenar datos temporalmente
struct_message_arm ultimosDatosBrazo;
struct_message_forearm ultimosDatosAntebrazo;

bool brazoInicializado = false;
bool antebrazoInicializado = false;
bool datosPendientes = false;  // Flag para indicar que hay datos nuevos para enviar

unsigned long tiempoUltimoBrazo = 0;
unsigned long tiempoUltimoAntebrazo = 0;
const unsigned long TIMEOUT_MS = 5000;  // Aumentado a 5 segundos

// ==========================================
// VARIABLES MQTT
// ==========================================
WiFiClientSecure espClient;
PubSubClient client(espClient);
unsigned long lastMQTTSendTime = 0;
const long mqttInterval = 2000;

// ==========================================
// VARIABLES PARA ALTERNAR MODOS
// ==========================================
enum SystemMode {
    MODE_ESPNOW_RECEIVE,  // Modo recepción ESP-NOW
    MODE_WIFI_SEND        // Modo envío MQTT
};

SystemMode currentMode = MODE_ESPNOW_RECEIVE;
unsigned long modeStartTime = 0;
const unsigned long ESPNOW_RECEIVE_DURATION = 1000;  // 3 segundos escuchando ESP-NOW
const unsigned long WIFI_SEND_DURATION = 1000;       // 3 segundos para WiFi/MQTT

// ==========================================
// FUNCIONES WIFI Y MQTT
// ==========================================
void conectarWiFi() {
    Serial.print("Conectando WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, wifi_password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Canal: ");
        Serial.println(WiFi.channel());
    } else {
        Serial.println("\nError conectando WiFi");
    }
}

void conectarMQTT() {
    espClient.setCACert(ca_cert);
    client.setServer(broker, port);
    
    int attempts = 0;
    while (!client.connected() && attempts < 5) {
        Serial.print("Conectando MQTT...");
        if (client.connect("ESP32_Master_Node", mqtt_user, mqtt_password)) {
            Serial.println("Conectado");
        } else {
            Serial.print("Error, rc=");
            Serial.print(client.state());
            Serial.println(" Reintentando...");
            delay(2000);
            attempts++;
        }
    }
}

void desconectarWiFi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi desconectado");
}

void iniciarModoESPNOW() {
    // Apagar WiFi para liberar el módulo
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Inicializar ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);
    
    Serial.println("Modo ESP-NOW: Escuchando datos...");
}

void detenerModoESPNOW() {
    esp_now_deinit();
    Serial.println("ESP-NOW detenido");
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
            datosPendientes = true;
            
            if (!brazoInicializado) {
                brazoInicializado = true;
                Serial.println("\n[ESP-NOW] BRAZO detectado");
            }
            
            Serial.print("\n[ESP-NOW] BRAZO: ");
            Serial.print("roll="); Serial.print(datosBrazo.roll_a, 1);
            Serial.print(", pitch="); Serial.print(datosBrazo.pitch_a, 1);
            Serial.print(", yaw="); Serial.print(datosBrazo.yaw_a, 1);
            Serial.print(", ecg="); Serial.println(datosBrazo.ecg);
        }
    }
    else if (len == sizeof(struct_message_forearm)) {
        struct_message_forearm datosAntebrazo;
        memcpy(&datosAntebrazo, incomingDataBytes, sizeof(datosAntebrazo));
        
        if (datosAntebrazo.node_id == 2) {
            ultimosDatosAntebrazo = datosAntebrazo;
            tiempoUltimoAntebrazo = millis();
            datosPendientes = true;
            
            if (!antebrazoInicializado) {
                antebrazoInicializado = true;
                Serial.println("\n[ESP-NOW] ANTEBRAZO detectado");
            }
            
            Serial.print("\n[ESP-NOW] ANTEBRAZO: ");
            Serial.print("roll="); Serial.print(datosAntebrazo.roll_f, 1);
            Serial.print(", pitch="); Serial.print(datosAntebrazo.pitch_f, 1);
            Serial.print(", yaw="); Serial.println(datosAntebrazo.yaw_f, 1);
        }
    }
}

// ==========================================
// FUNCIÓN PARA ENVIAR DATOS POR MQTT
// ==========================================
void enviarDatosMQTT() {
    // Siempre enviar los últimos datos recibidos, aunque estén viejos
    if (!brazoInicializado && !antebrazoInicializado) {
        Serial.println("[MQTT] No hay datos para enviar (ningún nodo inicializado)");
        return;
    }
    
    char msg[256];
    
    // Enviar datos del brazo si está inicializado, con sus valores (incluso si están viejos)
    if (brazoInicializado && antebrazoInicializado) {
        // Ambos nodos tienen datos
        snprintf(msg, sizeof(msg),
            "{\"roll_a\":%.2f,\"pitch_a\":%.2f,\"yaw_a\":%.2f,\"ecg\":%d,\"roll_f\":%.2f,\"pitch_f\":%.2f,\"yaw_f\":%.2f}",
            ultimosDatosBrazo.roll_a, ultimosDatosBrazo.pitch_a, ultimosDatosBrazo.yaw_a, ultimosDatosBrazo.ecg,
            ultimosDatosAntebrazo.roll_f, ultimosDatosAntebrazo.pitch_f, ultimosDatosAntebrazo.yaw_f);
    }
    else if (brazoInicializado) {
        // Solo brazo
        snprintf(msg, sizeof(msg),
            "{\"roll_a\":%.2f,\"pitch_a\":%.2f,\"yaw_a\":%.2f,\"ecg\":%d,\"roll_f\":0,\"pitch_f\":0,\"yaw_f\":0}",
            ultimosDatosBrazo.roll_a, ultimosDatosBrazo.pitch_a, ultimosDatosBrazo.yaw_a, ultimosDatosBrazo.ecg);
    }
    else if (antebrazoInicializado) {
        // Solo antebrazo
        snprintf(msg, sizeof(msg),
            "{\"roll_a\":0,\"pitch_a\":0,\"yaw_a\":0,\"ecg\":0,\"roll_f\":%.2f,\"pitch_f\":%.2f,\"yaw_f\":%.2f}",
            ultimosDatosAntebrazo.roll_f, ultimosDatosAntebrazo.pitch_f, ultimosDatosAntebrazo.yaw_f);
    }
    
    // Verificar conexión MQTT
    if (!client.connected()) {
        conectarMQTT();
    }
    
    if (client.connected() && client.publish(topicPublish, msg)) {
        Serial.print("[MQTT] Enviado: ");
        Serial.println(msg);
        
        // Mostrar tiempo desde última recepción
        unsigned long ahora = millis();
        if (brazoInicializado) {
            Serial.print("  Último brazo: ");
            Serial.print(ahora - tiempoUltimoBrazo);
            Serial.println(" ms atrás");
        }
        if (antebrazoInicializado) {
            Serial.print("  Último antebrazo: ");
            Serial.print(ahora - tiempoUltimoAntebrazo);
            Serial.println(" ms atrás");
        }
        
        datosPendientes = false;
    } else {
        Serial.println("[MQTT] Error al publicar - Cliente no conectado o error de publicación");
        if (!client.connected()) {
            Serial.println("  Estado MQTT: Desconectado");
        }
    }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    Serial.println("\n=== Nodo Puente ESP-NOW <-> MQTT ===");
    Serial.println("Modo: Alternancia entre recepción y envío");
    Serial.println("Enviará los últimos datos recibidos aunque estén viejos");
    
    // Iniciar en modo ESP-NOW
    currentMode = MODE_ESPNOW_RECEIVE;
    modeStartTime = millis();
    iniciarModoESPNOW();
}

// ==========================================
// LOOP PRINCIPAL - ALTERNANCIA DE MODOS
// ==========================================
void loop() {
    unsigned long ahora = millis();
    
    switch(currentMode) {
        case MODE_ESPNOW_RECEIVE:
            // Escuchar ESP-NOW durante el tiempo configurado
            if (ahora - modeStartTime >= ESPNOW_RECEIVE_DURATION) {
                // Cambiar a modo WiFi/MQTT para enviar
                Serial.println("\n=== Cambiando a modo MQTT ===");
                detenerModoESPNOW();
                
                // Conectar WiFi y MQTT
                conectarWiFi();
                conectarMQTT();
                
                // Enviar datos (siempre, aunque estén viejos)
                Serial.println("\n[INFO] Enviando últimos datos disponibles...");
                enviarDatosMQTT();
                
                // Desconectar WiFi para liberar recursos
                desconectarWiFi();
                
                // Cambiar modo
                currentMode = MODE_WIFI_SEND;
                modeStartTime = ahora;
            }
            break;
            
        case MODE_WIFI_SEND:
            // Esperar un momento antes de volver a ESP-NOW
            if (ahora - modeStartTime >= WIFI_SEND_DURATION) {
                Serial.println("\n=== Cambiando a modo ESP-NOW ===");
                iniciarModoESPNOW();
                
                currentMode = MODE_ESPNOW_RECEIVE;
                modeStartTime = ahora;
            }
            break;
    }
    
    // Mantener el loop MQTT si estamos en ese modo
    if (currentMode == MODE_WIFI_SEND && client.connected()) {
        client.loop();
    }
    
    delay(10);
}