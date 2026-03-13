// ===============================
// CONFIGURACIÓN DE PINES
// ===============================
const int PIN_ECG = 2;
const int PIN_LOP = 3;
const int PIN_LON = 5;

int adc_center = 0;

float envelope = 0;
float alpha = 0.1;

// ===============================
// CALIBRACIÓN
// ===============================
void calibrate() {

  long sum = 0;

  for (int i = 0; i < 500; i++) {
    sum += analogRead(PIN_ECG);
    delay(2);
  }

  adc_center = sum / 500;

  Serial.print("Centro ADC: ");
  Serial.println(adc_center);
}

// ===============================
// SETUP
// ===============================
void setup() {

  Serial.begin(115200);

  pinMode(PIN_ECG, INPUT);
  pinMode(PIN_LOP, INPUT);
  pinMode(PIN_LON, INPUT);

  delay(2000);

  calibrate();
}

// ===============================
// LOOP
// ===============================
void loop() {

  if (digitalRead(PIN_LOP) == HIGH || digitalRead(PIN_LON) == HIGH) {

    Serial.println("0,0,100");
    delay(10);
    return;
  }

  int raw = analogRead(PIN_ECG);

  int centered = raw - adc_center;

  int rectified = abs(centered);

  envelope = alpha * rectified + (1 - alpha) * envelope;

  int muscle = map(envelope, 0, 400, 0, 100);

  muscle = constrain(muscle, 0, 100);

  Serial.print(muscle);
  Serial.print(",");
  Serial.print(0);
  Serial.print(",");
  Serial.println(100);

  delay(2);
}