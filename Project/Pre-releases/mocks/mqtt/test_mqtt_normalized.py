import json
import ssl
import paho.mqtt.client as mqtt
import tkinter as tk

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
-----END CERTIFICATE-----"""

with open("ca_cert.pem", "w") as f:
    f.write(CA_CERT)

# ==========================================
# MQTT CLIENT
# ==========================================
client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)

client.tls_set(
    ca_certs="ca_cert.pem",
    tls_version=ssl.PROTOCOL_TLS_CLIENT
)

client.connect(BROKER, PORT)
client.loop_start()

print("✅ MQTT conectado")


# ==========================================
# TKINTER
# ==========================================
root = tk.Tk()
root.title("MQTT IMU Controller")
root.geometry("500x700")

sliders = {}

def add_slider(name, minv, maxv):
    tk.Label(root, text=name, font=("Arial", 12)).pack()
    s = tk.Scale(
        root,
        from_=minv,
        to=maxv,
        orient="horizontal",
        length=400,
        resolution=1
    )
    s.pack()
    sliders[name] = s

# Brazo
add_slider("roll_a", -90, 90)
add_slider("pitch_a", -90, 90)
add_slider("yaw_a", -90, 90)

# Antebrazo
add_slider("roll_f", -90, 90)
add_slider("pitch_f", -90, 90)
add_slider("yaw_f", -90, 90)

# ECG
add_slider("ecg", 0, 4095)

# ==========================================
# PUBLICAR
# ==========================================
sending = True
def publish_data():

    if sending:

        data = {}

        for k in sliders:
            value = sliders[k].get()

            if "roll_f" in k or "yaw_f" in k:
                base_key = k.replace("_f", "_a")
                value += sliders[base_key].get()

            data[k] = value

        payload = json.dumps(data)

        client.publish(TOPIC, payload)

        print("📡", payload)

    root.after(500, publish_data)

def toggle_sending():
    global sending

    sending = not sending

    if sending:
        button.config(text="⏸ Detener")
        print("▶ Enviando datos")
    else:
        button.config(text="▶ Reanudar")
        print("⏸ Envío detenido")

button = tk.Button(
    root,
    text="⏸ Detener",
    font=("Arial", 12),
    command=toggle_sending
)

button.pack(pady=10)

publish_data()


# ==========================================
# CIERRE LIMPIO
# ==========================================
def on_close():
    client.loop_stop()
    client.disconnect()
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_close)

root.mainloop()