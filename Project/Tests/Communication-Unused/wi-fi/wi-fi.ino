#include <WiFi.h>

const char* ssid     = "AsusE";          // Nombre de la red WiFi
const char* password = "23011edpi";      // Contraseña de la red WiFi

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Conectando a WiFi...");
  WiFi.begin(ssid, password);

  // Esperar hasta que se conecte
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Conectado a la red WiFi!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Aquí puedes poner el código que necesite conexión a internet
}
