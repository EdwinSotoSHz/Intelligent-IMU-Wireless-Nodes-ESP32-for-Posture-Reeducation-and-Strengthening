#define RXD2 16
#define TXD2 17

HardwareSerial sim800l(2);

void setup()
{
  Serial.begin(115200);
  sim800l.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("Inicializando...");
  delay(3000);

  // Modo texto
  sim800l.println("AT+CMGF=1");
  delay(1000);

  // Notificación inmediata de SMS entrante
  sim800l.println("AT+CNMI=2,2,0,0,0");
  delay(1000);

  Serial.println("Listo para recibir SMS...");
}

void loop()
{
  while (sim800l.available())
  {
    Serial.write(sim800l.read());
  }
}