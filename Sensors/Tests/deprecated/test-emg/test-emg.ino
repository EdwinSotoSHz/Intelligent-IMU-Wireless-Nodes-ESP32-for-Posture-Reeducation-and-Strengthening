// Código de diagnóstico completo
#define LO_PLUS 27
#define LO_MINUS 26
#define EMG_PIN 25
#define TEST_PIN 32  // Pin extra para comparar

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  pinMode(LO_PLUS, INPUT);
  pinMode(LO_MINUS, INPUT);
  
  Serial.println("=== DIAGNÓSTICO EMG ===");
}

void loop() {
  // Lectura del sensor EMG
  int emgValue = analogRead(EMG_PIN);
  
  // Lectura de un pin sin conectar (referencia)
  int testValue = analogRead(TEST_PIN);
  
  // Estado de leads
  int loPlus = digitalRead(LO_PLUS);
  int loMinus = digitalRead(LO_MINUS);
  
  Serial.print("Sensor EMG (GPIO25): ");
  Serial.print(emgValue);
  Serial.print(" | Pin prueba (GPIO32): ");
  Serial.print(testValue);
  Serial.print(" | LO+: ");
  Serial.print(loPlus);
  Serial.print(" LO-: ");
  Serial.println(loMinus);
  
  if(emgValue == 4095) {
    Serial.println("⚠️  ALERTA: Sensor saturado a 4095");
    Serial.println("Posibles causas:");
    Serial.println("1. Sensor alimentado con 5V (debe ser 3.3V)");
    Serial.println("2. OUTPUT conectado a 3.3V por error");
    Serial.println("3. Sensor dañado");
    Serial.println("4. GND no conectado");
    Serial.println("-------------------");
  }
  
  delay(1000);
}