/*
| FLEXÓMETRO - CONEXIONES A ESP32                    |
|                                                    |
|   [3.3V] <---> [ FLEX ] <-|-> [ 10kΩ ] <---> GND   |
|                           |                        |
|                         GPIO25                     |

Explicación:                                       
- El flexómetro cambia su resistencia al flexionarse
- Resistencia de 10kΩ para convertirlo en variación de voltaje                           
- GPIO25 lee el voltaje en el punto medio (ADC)     
*/

const int flexPin = 25;  // Pin analógico donde está conectado el flexómetro
int value;               // Variable para almacenar la lectura

void setup() {
  Serial.begin(9600);    // Iniciar comunicación serial
}

void loop() {
  value = analogRead(flexPin);  // Leer valor analógico (0 - 4095 en ESP32)
  Serial.println(value);        // Mostrar valor en el monitor serial
  delay(100);                   // Pequeña pausa
}