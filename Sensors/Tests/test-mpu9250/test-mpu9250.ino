#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>

#define SDA_PIN 33
#define SCL_PIN 32
#define MPU_ADDR 0x68

const char* ssid = "AsusE";
const char* password = "23011edpi";

WebServer server(80);
WebSocketsServer webSocket(81);

float roll = 0, pitch = 0, yaw = 0;
float gyroX, gyroY, gyroZ;
float accX, accY, accZ;

float gyroX_offset = 0;
float gyroY_offset = 0;
float gyroZ_offset = 0;

unsigned long prevTime;

const float alpha = 0.98;  // filtro complementario

// ================== HTML ==================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>MPU 3D</title>
<style>
body { margin:0; overflow:hidden; background:#111; color:white; font-family:Arial }
#info {
 position:absolute;
 top:10px;
 left:10px;
 background:rgba(0,0,0,0.6);
 padding:10px;
 border-radius:8px;
}
</style>
</head>
<body>

<div id="info">
Roll: <span id="r">0</span><br>
Pitch: <span id="p">0</span><br>
Yaw: <span id="y">0</span>
</div>

<script type="module">
import * as THREE from 'https://unpkg.com/three@0.180.0/build/three.module.js';

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, innerWidth/innerHeight, 0.1, 1000);
const renderer = new THREE.WebGLRenderer({antialias:true});
renderer.setSize(innerWidth, innerHeight);
document.body.appendChild(renderer.domElement);

const light = new THREE.DirectionalLight(0xffffff, 1);
light.position.set(2,2,5);
scene.add(light);

const geometry = new THREE.BoxGeometry();
const material = new THREE.MeshStandardMaterial({color:0x00ffaa});
const cube = new THREE.Mesh(geometry, material);
scene.add(cube);

camera.position.z = 3;

const socket = new WebSocket('ws://' + location.hostname + ':81/');

socket.onmessage = (event)=>{
 const data = JSON.parse(event.data);

 // Ajuste de ejes para coincidir con Three.js
 cube.rotation.x = data.pitch * Math.PI/180;
 cube.rotation.y = data.roll * Math.PI/180;
 cube.rotation.z = data.yaw * Math.PI/180;

 document.getElementById("r").textContent = data.roll.toFixed(2);
 document.getElementById("p").textContent = data.pitch.toFixed(2);
 document.getElementById("y").textContent = data.yaw.toFixed(2);
};

function animate(){
 requestAnimationFrame(animate);
 renderer.render(scene, camera);
}
animate();
</script>
</body>
</html>
)rawliteral";

// ================== MPU ==================

void setupMPU(){
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void readMPU(){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  accX = (Wire.read()<<8|Wire.read()) / 16384.0;
  accY = (Wire.read()<<8|Wire.read()) / 16384.0;
  accZ = (Wire.read()<<8|Wire.read()) / 16384.0;
  Wire.read(); Wire.read();

  gyroX = (Wire.read()<<8|Wire.read()) / 131.0 - gyroX_offset;
  gyroY = (Wire.read()<<8|Wire.read()) / 131.0 - gyroY_offset;
  gyroZ = (Wire.read()<<8|Wire.read()) / 131.0 - gyroZ_offset;
}

void calibrateGyro(){
  Serial.println("Calibrando gyro...");
  for(int i=0;i<2000;i++){
    readMPU();
    gyroX_offset += gyroX;
    gyroY_offset += gyroY;
    gyroZ_offset += gyroZ;
    delay(2);
  }
  gyroX_offset /= 2000;
  gyroY_offset /= 2000;
  gyroZ_offset /= 2000;
  Serial.println("Calibración lista");
}

void calculateAngles(){
  unsigned long currentTime = micros();
  float dt = (currentTime - prevTime) / 1000000.0;
  prevTime = currentTime;

  float accRoll = atan2(accY, accZ) * 180/PI;
  float accPitch = atan2(-accX, sqrt(accY*accY + accZ*accZ)) * 180/PI;

  roll  = alpha*(roll  + gyroX*dt) + (1-alpha)*accRoll;
  pitch = alpha*(pitch + gyroY*dt) + (1-alpha)*accPitch;
  yaw  += gyroZ*dt;
}

void handleRoot(){
  server.send_P(200,"text/html",index_html);
}

void setup(){
  Serial.begin(115200);
  setupMPU();
  delay(1000);
  calibrateGyro();

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED);

  server.on("/",handleRoot);
  server.begin();
  webSocket.begin();

  prevTime = micros();
}

void loop(){
  server.handleClient();
  webSocket.loop();

  readMPU();
  calculateAngles();

  String json = "{\"roll\":"+String(roll,2)+
                ",\"pitch\":"+String(pitch,2)+
                ",\"yaw\":"+String(yaw,2)+"}";

  webSocket.broadcastTXT(json);
}