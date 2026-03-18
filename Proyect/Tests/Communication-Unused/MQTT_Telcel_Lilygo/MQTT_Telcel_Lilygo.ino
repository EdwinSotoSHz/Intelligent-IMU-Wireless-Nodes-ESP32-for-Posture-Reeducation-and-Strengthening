/**
 * PROYECTO: Telemetría IoT vía Red Celular (LTE-M / NB-IoT)
 * INSTITUCIÓN: Universidad Autónoma del Estado de Hidalgo (UAEH)
 * PROFESOR: MGTI. Saul Isai Soto Ortiz
 */

//#define TINY_GSM_MODEM_SIM7000 
#define TINY_GSM_MODEM_SIM7070

#include <TinyGsmClient.h>
#include <PubSubClient.h>

// --- CONFIGURACIÓN DE ACCESO A LA RED ---
const char apn[]      = "internet.itelcel.com"; 
const char gprsUser[] = ""; 
const char gprsPass[] = "";

// --- MQTT ---
const char* broker = "broker.hivemq.com";
const char* topicPublish = "itics/6b/masternode";
const int   port = 1883;

// --- PINES ---
#define UART_BAUD   115200
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4
#define LED_INDICATOR 12

HardwareSerial ModemSerial(1);

TinyGsm modem(ModemSerial);
TinyGsmClient client(modem);
PubSubClient  mqtt(client);

// -------- STRUCT --------
typedef struct struct_message_completo {
    float roll_f, pitch_f, yaw_f;
    int ecg;
    float roll_a, pitch_a, yaw_a;
} struct_message_completo;

// -------- ENCENDER MODEM --------
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

// -------- SETUP --------
void setup() {
  Serial.begin(115200);
  
  encenderModem();

  ModemSerial.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(3000);

  Serial.println("PASO 1: Verificando comunicación AT...");
  if (!modem.restart()) {
    Serial.println("CRÍTICO: El módem no responde.");
    while(true);
  }

  Serial.print("PASO 2: Buscando señal de torre celular...");
  if (!modem.waitForNetwork()) {
    Serial.println(" ERROR: Sin cobertura.");
    return;
  }
  Serial.println(" OK");

  Serial.print("PASO 3: Activando sesión de datos...");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" ERROR APN.");
    return;
  }

  Serial.print("IP: ");
  Serial.println(modem.getLocalIP());

  mqtt.setServer(broker, port);

  randomSeed(analogRead(0)); // para random más variable
}

// -------- LOOP --------
void loop() {
  if (!mqtt.connected()) {
    reconectarMQTT();
  }
  
  mqtt.loop();

  static unsigned long ultimaMarca = 0;
  if (millis() - ultimaMarca > 20000) {
    ultimaMarca = millis();

    // -------- GENERAR DATOS --------
    struct_message_completo data;

    data.roll_f  = random(-900, 900) / 10.0;
    data.pitch_f = random(-900, 900) / 10.0;
    data.yaw_f   = random(-1800, 1800) / 10.0;

    data.ecg = random(600, 900);

    data.roll_a  = random(-900, 900) / 10.0;
    data.pitch_a = random(-900, 900) / 10.0;
    data.yaw_a   = random(-1800, 1800) / 10.0;

    // -------- CREAR JSON --------
    String payload = "{";
    payload += "\"roll_f\":" + String(data.roll_f, 2) + ",";
    payload += "\"pitch_f\":" + String(data.pitch_f, 2) + ",";
    payload += "\"yaw_f\":" + String(data.yaw_f, 2) + ",";
    payload += "\"ecg\":" + String(data.ecg) + ",";
    payload += "\"roll_a\":" + String(data.roll_a, 2) + ",";
    payload += "\"pitch_a\":" + String(data.pitch_a, 2) + ",";
    payload += "\"yaw_a\":" + String(data.yaw_a, 2);
    payload += "}";

    Serial.println(">>> Enviando JSON:");
    Serial.println(payload);

    if (mqtt.publish(topicPublish, payload.c_str())) {
      Serial.println("MENSAJE RECIBIDO POR EL BROKER");
    } else {
      Serial.println("Error de transmisión");
    }
  }
}

// -------- RECONEXIÓN --------
void reconectarMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Intentando acceso al Broker MQTT...");
    String clienteID = "Proyecto6B" + String(random(100, 999));
    
    if (mqtt.connect(clienteID.c_str())) {
      Serial.println(" CONECTADO.");
    } else {
      Serial.print(" FAILED (Code: ");
      Serial.print(mqtt.state());
      Serial.println("). Reintento...");
      delay(5000);
    }
  }
}