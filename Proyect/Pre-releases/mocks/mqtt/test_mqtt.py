import json
import random
import time
import ssl
import paho.mqtt.client as mqtt

# ==========================================
# CONFIGURACIÓN MQTT
# ==========================================
BROKER = "oc051111.ala.us-east-1.emqxsl.com"
PORT = 8883
TOPIC = "test01"
USERNAME = "edpi"
PASSWORD = "edpi"

# ==========================================
# CERTIFICADO CA
# ==========================================
CA_CERT = """-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
"""

# Guardar certificado en archivo temporal
with open("ca_cert.pem", "w") as f:
    f.write(CA_CERT)

# ==========================================
# CALLBACKS
# ==========================================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Conectado al broker MQTT")
    else:
        print(f"❌ Error de conexión: {rc}")

def on_publish(client, userdata, mid):
    print(f"📤 Mensaje enviado (id: {mid})")

# ==========================================
# CLIENTE MQTT
# ==========================================
client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)

# TLS
client.tls_set(
    ca_certs="ca_cert.pem",
    certfile=None,
    keyfile=None,
    tls_version=ssl.PROTOCOL_TLS_CLIENT
)

client.on_connect = on_connect
client.on_publish = on_publish

# Conectar
client.connect(BROKER, PORT)
client.loop_start()

# ==========================================
# LOOP DE ENVÍO
# ==========================================
def next_value(prev, step=10, min_val=-90, max_val=90):
    change = random.uniform(-step, step)
    new_val = prev + change
    return max(min(new_val, max_val), min_val)

try:
    # Estado inicial
    state = {
        "roll_f": 0.0,
        "pitch_f": 0.0,
        "yaw_f": 0.0,
        "roll_a": 0.0,
        "pitch_a": 0.0,
        "yaw_a": 0.0,
    }
    iteration = 0
    while True:
        iteration += 1

        # --------------------------------------
        # FASE 1: primeros 3 mensajes en 0
        # --------------------------------------
        if iteration <= 3:
            data = {
                "roll_f": 0.0,
                "pitch_f": 0.0,
                "yaw_f": 0.0,
                "ecg": random.randint(0, 4095),
                "roll_a": 0.0,
                "pitch_a": 0.0,
                "yaw_a": 0.0,
            }

        else:
            # --------------------------------------
            # FASE 2 y 3: movimiento progresivo
            # --------------------------------------
            step = 3 if iteration <= 8 else 10  # 5 iteraciones suaves después del 0

            for key in state:
                state[key] = round(next_value(state[key], step=step), 2)

            data = {
                "roll_f": state["roll_f"],
                "pitch_f": state["pitch_f"],
                "yaw_f": state["yaw_f"],
                "ecg": random.randint(0, 4095),
                "roll_a": state["roll_a"],
                "pitch_a": state["pitch_a"],
                "yaw_a": state["yaw_a"],
            }

        payload = json.dumps(data)
        print(f"📡 Iter {iteration}:", payload)

        client.publish(TOPIC, payload)
        time.sleep(1)

except KeyboardInterrupt:
    print("🛑 Detenido por el usuario")
    client.loop_stop()
    client.disconnect()