/*
| MAX30102     |  ESP32          |
| VIN          |  3.3V           |
| GND          |  GND            |
| SCL          |  GPIO27         |
| SDA          |  GPIO26         |
*/

#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

// Configuración
#define SDA_PIN 26
#define SCL_PIN 27

// Variables para BPM
long irValue;
long irFiltered = 0;
long previousIr = 0;

unsigned long lastBeat = 0;
float bpm = 0;
float bpmAvg = 0;

const byte RATE_SIZE = 8; 
byte rates[RATE_SIZE];
byte rateSpot = 0;

void setup()
{
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD))
  {
    Serial.println("MAX30102 no encontrado");
    while (1);
  }

  Serial.println("MAX30102 listo");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x3F); // Más potencia IR para mejor señal
}

void loop()
{
  irValue = particleSensor.getIR();

  // Detectar si hay dedo
  if (irValue < 50000)
  {
    Serial.println("Coloca el dedo");
    delay(500);
    return;
  }

  // Filtro simple (promedio móvil)
  irFiltered = (previousIr * 0.9) + (irValue * 0.1);
  previousIr = irFiltered;

  // Detección básica de pico
  if (irFiltered > 60000 && millis() - lastBeat > 300)
  {
    unsigned long delta = millis() - lastBeat;
    lastBeat = millis();

    bpm = 60.0 / (delta / 1000.0);

    if (bpm > 40 && bpm < 200)
    {
      rates[rateSpot++] = (byte)bpm;
      rateSpot %= RATE_SIZE;

      bpmAvg = 0;
      for (byte i = 0; i < RATE_SIZE; i++)
        bpmAvg += rates[i];
      bpmAvg /= RATE_SIZE;

      Serial.print("BPM: ");
      Serial.print(bpm);
      Serial.print(" | Promedio: ");
      Serial.println(bpmAvg);
    }
  }

  delay(10);
}