#define FLEX_PIN 13   // GPIO 13 (D13)

void setup() {
  Serial.begin(115200);

  // Configuración del ADC
  analogReadResolution(12);        // 12 bits (0 - 4095)
  analogSetAttenuation(ADC_11db);  // Rango hasta ~3.3V
}

void loop() {
  int valorADC = analogRead(FLEX_PIN);

  Serial.print("Valor ADC: ");
  Serial.println(valorADC);

  delay(500);
}