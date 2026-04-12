// NO se usa SoftwareSerial en ESP32

#define RXD2 16
#define TXD2 17

HardwareSerial sim800l(2);  // Usamos UART2

void setup()
{
  // Comunicación con Monitor Serial
  Serial.begin(115200);
  
  // Comunicación con SIM800L
  sim800l.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("Initializing..."); 
  delay(3000);  // Tiempo para que el módulo arranque

  sim800l.println("AT"); 
  updateSerial();

  sim800l.println("AT+CMGF=1"); // Configurar modo texto
  updateSerial();

  sim800l.println("AT+CMGS=\"+527737399765\"");
  updateSerial();

  sim800l.print("SMS Test"); 
  updateSerial();

  sim800l.write(26); // CTRL+Z para enviar SMS
  delay(5000);

  Serial.println("Mensaje enviado.");
}

void loop()
{
}

void updateSerial()  
{
  delay(500);

  while (Serial.available()) 
  {
    sim800l.write(Serial.read());
  }

  while(sim800l.available()) 
  {
    Serial.write(sim800l.read());
  }
}