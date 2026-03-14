// ===============================
// CONFIGURACIÓN DE PINES
// ===============================
const int PIN_ECG = 2;
const int PIN_LOP = 3;
const int PIN_LON = 5;

// ===============================
// SETUP
// ===============================
void setup() {

  Serial.begin(115200);

  pinMode(PIN_ECG, INPUT);
  pinMode(PIN_LOP, INPUT);
  pinMode(PIN_LON, INPUT);
}

// ===============================
// LOOP PRINCIPAL
// ===============================
void loop() {

  // Leer señal ECG
  int ecg = analogRead(PIN_ECG);

  // Verificar si los electrodos están desconectados
  if (digitalRead(PIN_LOP) == HIGH || digitalRead(PIN_LON) == HIGH) {

    // Señal en 0 si están desconectados
    Serial.println("0,0,4095");

  } else {

    // Salida para Serial Plotter
    // ECG , LIMITE_MIN , LIMITE_MAX
    Serial.print(ecg);
    Serial.print(",");
    Serial.print(0);      // límite inferior
    Serial.print(",");
    Serial.println(4095); // límite superior
  }

  delay(10);
}