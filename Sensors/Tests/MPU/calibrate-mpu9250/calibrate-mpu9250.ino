#include <Wire.h>

#define SDA_PIN 33
#define SCL_PIN 32
#define MPU_ADDR 0x68

int16_t ax, ay, az;
int16_t gx, gy, gz;

long sumAX=0, sumAY=0, sumAZ=0;
long sumGX=0, sumGY=0, sumGZ=0;

float meanAX, meanAY, meanAZ;
float meanGX, meanGY, meanGZ;

const int samples = 5000;

void writeReg(uint8_t reg, uint8_t val){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void readMPU(){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR,14,true);

  ax = Wire.read()<<8 | Wire.read();
  ay = Wire.read()<<8 | Wire.read();
  az = Wire.read()<<8 | Wire.read();
  Wire.read(); Wire.read();
  gx = Wire.read()<<8 | Wire.read();
  gy = Wire.read()<<8 | Wire.read();
  gz = Wire.read()<<8 | Wire.read();
}

void setup(){
  Serial.begin(115200);
  Wire.begin(SDA_PIN,SCL_PIN);

  writeReg(0x6B,0x00);  // Wake
  writeReg(0x1C,0x00);  // ±2g
  writeReg(0x1B,0x00);  // ±250 dps

  delay(2000);

  Serial.println("NO MOVER EL SENSOR...");
  delay(3000);

  for(int i=0;i<samples;i++){
    readMPU();
    sumAX+=ax;
    sumAY+=ay;
    sumAZ+=az;
    sumGX+=gx;
    sumGY+=gy;
    sumGZ+=gz;
    delay(2);
  }

  meanAX=sumAX/(float)samples;
  meanAY=sumAY/(float)samples;
  meanAZ=sumAZ/(float)samples;

  meanGX=sumGX/(float)samples;
  meanGY=sumGY/(float)samples;
  meanGZ=sumGZ/(float)samples;

  Serial.println("===== PROMEDIOS RAW =====");
  Serial.print("AX: "); Serial.println(meanAX);
  Serial.print("AY: "); Serial.println(meanAY);
  Serial.print("AZ: "); Serial.println(meanAZ);

  Serial.print("GX: "); Serial.println(meanGX);
  Serial.print("GY: "); Serial.println(meanGY);
  Serial.print("GZ: "); Serial.println(meanGZ);

  Serial.println("===== CONVERTIDO =====");

  Serial.print("AX g: "); Serial.println(meanAX/16384.0,6);
  Serial.print("AY g: "); Serial.println(meanAY/16384.0,6);
  Serial.print("AZ g: "); Serial.println(meanAZ/16384.0,6);

  Serial.print("GX dps: "); Serial.println(meanGX/131.0,6);
  Serial.print("GY dps: "); Serial.println(meanGY/131.0,6);
  Serial.print("GZ dps: "); Serial.println(meanGZ/131.0,6);
}

void loop(){}