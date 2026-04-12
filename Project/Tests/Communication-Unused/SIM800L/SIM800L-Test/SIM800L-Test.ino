#define RXD2 16
#define TXD2 17

HardwareSerial sim800l(2);  // Usamos UART2

void setup()
{
  Serial.begin(115200);
  sim800l.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("Initializing...");
  delay(1000);

  sim800l.println("AT");
  updateSerial();

  sim800l.println("AT+CSQ");
  updateSerial();

  sim800l.println("AT+CCID");
  updateSerial();

  sim800l.println("AT+CREG?");
  updateSerial();

  sim800l.println("AT+CBC");
  updateSerial();
}

void loop()
{
  updateSerial();
}

void updateSerial()
{
  delay(500);

  while (Serial.available()) 
  {
    sim800l.write(Serial.read());
  }

  while (sim800l.available()) 
  {
    Serial.write(sim800l.read());
  }
}