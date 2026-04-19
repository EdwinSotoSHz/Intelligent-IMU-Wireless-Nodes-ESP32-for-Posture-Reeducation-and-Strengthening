#include <Wire.h>

#define SDA_PIN 1
#define SCL_PIN 0
#define MPU_ADDR 0x68

long axSum=0, aySum=0, azSum=0;
long gxSum=0, gySum=0, gzSum=0;

int samples = 0;

void readMPU(int16_t &ax, int16_t &ay, int16_t &az,
             int16_t &gx, int16_t &gy, int16_t &gz) {

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

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN,SCL_PIN);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission();

    Serial.println("NO MOVER - calibrando...");
    delay(1000);
}

void loop() {

    if(samples < 10000){

        int16_t ax, ay, az, gx, gy, gz;
        readMPU(ax, ay, az, gx, gy, gz);

        axSum += ax;
        aySum += ay;
        azSum += az;

        gxSum += gx;
        gySum += gy;
        gzSum += gz;

        samples++;
        delay(2);

    } else {

        float axAvg = axSum/(float)samples;
        float ayAvg = aySum/(float)samples;
        float azAvg = azSum/(float)samples;

        float gxAvg = gxSum/(float)samples;
        float gyAvg = gySum/(float)samples;
        float gzAvg = gzSum/(float)samples;

        // 🔑 Corrección clave: quitar gravedad en Z
        float accBiasX = axAvg;
        float accBiasY = ayAvg;
        float accBiasZ = azAvg - 16384.0;

        Serial.println("=== BIAS CALIBRADOS ===");

        Serial.print("GYRO_BIAS_X = "); Serial.println(gxAvg);
        Serial.print("GYRO_BIAS_Y = "); Serial.println(gyAvg);
        Serial.print("GYRO_BIAS_Z = "); Serial.println(gzAvg);

        Serial.println();

        Serial.print("ACC_BIAS_X = "); Serial.println(accBiasX);
        Serial.print("ACC_BIAS_Y = "); Serial.println(accBiasY);
        Serial.print("ACC_BIAS_Z = "); Serial.println(accBiasZ);

        Serial.println("======================");

        while(1);
    }
}