import json
import random
import time
import ssl
import paho.mqtt.client as mqtt
import math

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

with open("ca_cert.pem", "w") as f:
    f.write(CA_CERT)

# ==========================================
# CALLBACKS
# ==========================================
def on_connect(client, userdata, flags, rc):
    print("✅ Conectado" if rc == 0 else f"❌ Error: {rc}")

def on_publish(client, userdata, mid):
    print(f"📤 {mid}")

# ==========================================
# CLIENTE MQTT
# ==========================================
client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)

client.tls_set(
    ca_certs="ca_cert.pem",
    tls_version=ssl.PROTOCOL_TLS_CLIENT
)

client.on_connect = on_connect
client.on_publish = on_publish

client.connect(BROKER, PORT)
client.loop_start()

# ==========================================
# LOOP
# ==========================================
try:
    iteration = 0
    t = 0
    dt = 0.25

    while True:
        iteration += 1

        if iteration <= 3:
            data = {
                "roll_f": 0.0, "pitch_f": 0.0, "yaw_f": 0.0,
                "roll_a": 0.0, "pitch_a": 0.0, "yaw_a": 0.0,
                "ecg": random.randint(0, 4095),
            }

        else:
            t += dt

            # =========================
            # BRAZO (más yaw y pitch)
            # =========================
            pitch_a = 35 * math.sin(t)
            yaw_a   = 15 * math.sin(t * 0.7 + 0.5)
            roll_a  = 6  * math.sin(t * 0.5)

            # =========================
            # ANTEBRAZO (más roll)
            # =========================
            pitch_f = pitch_a + 35 * math.sin(t + 0.2)
            roll_f  = 15 * math.sin(t * 1.2)       # fuerte aquí
            yaw_f   = 8  * math.sin(t * 0.8 + 1)   # leve acompañamiento

            data = {
                "roll_f": round(roll_f, 2),
                "pitch_f": round(pitch_f, 2),
                "yaw_f": round(yaw_f, 2),
                "roll_a": round(roll_a, 2),
                "pitch_a": round(pitch_a, 2),
                "yaw_a": round(yaw_a, 2),
                "ecg": random.randint(0, 4095),
            }

        payload = json.dumps(data)
        print(f"📡 Iter {iteration}:", payload)

        client.publish(TOPIC, payload)
        time.sleep(1)

except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()