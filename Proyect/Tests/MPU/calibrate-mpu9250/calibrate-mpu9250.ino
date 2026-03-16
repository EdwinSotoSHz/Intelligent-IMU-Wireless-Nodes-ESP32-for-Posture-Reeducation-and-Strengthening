#include <Wire.h>

#define SDA_PIN 1
#define SCL_PIN 0
#define MPU_ADDR 0x68

long axSum=0, aySum=0, azSum=0;
long gxSum=0, gySum=0, gzSum=0;

int samples = 0;

void readMPU() {

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);

    Wire.requestFrom(MPU_ADDR,14,true);

    int16_t ax = Wire.read()<<8 | Wire.read();
    int16_t ay = Wire.read()<<8 | Wire.read();
    int16_t az = Wire.read()<<8 | Wire.read();

    Wire.read(); Wire.read();

    int16_t gx = Wire.read()<<8 | Wire.read();
    int16_t gy = Wire.read()<<8 | Wire.read();
    int16_t gz = Wire.read()<<8 | Wire.read();

    axSum += ax;
    aySum += ay;
    azSum += az;

    gxSum += gx;
    gySum += gy;
    gzSum += gz;

    samples++;
}

void setup() {

    Serial.begin(115200);
    Wire.begin(SDA_PIN,SCL_PIN);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission();

    Serial.println("No mover el sensor...");
}

void loop() {

    if(samples < 10000){

        readMPU();
        delay(2);

    } else {

        Serial.println("RESULTADOS:");

        Serial.print("ax avg = ");
        Serial.println(axSum/(float)samples);

        Serial.print("ay avg = ");
        Serial.println(aySum/(float)samples);

        Serial.print("az avg = ");
        Serial.println(azSum/(float)samples);

        Serial.print("gx avg = ");
        Serial.println(gxSum/(float)samples);

        Serial.print("gy avg = ");
        Serial.println(gySum/(float)samples);

        Serial.print("gz avg = ");
        Serial.println(gzSum/(float)samples);

        Serial.println("-----");

        while(1);
    }
}