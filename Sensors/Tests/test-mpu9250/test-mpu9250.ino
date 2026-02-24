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
WebSocketsServer webSocket = WebSocketsServer(81);

float roll, pitch, yaw;
float gyroX, gyroY, gyroZ;
float accX, accY, accZ;
unsigned long prevTime;
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>MPU 3D</title>
<style>
  body { margin:0; overflow:hidden; background:#000; }
</style>
</head>
<body>

<script type="module">

import * as THREE from 'https://unpkg.com/three@0.180.0/build/three.module.js';

const scene = new THREE.Scene();

const camera = new THREE.PerspectiveCamera(
  75,
  window.innerWidth / window.innerHeight,
  0.1,
  1000
);

const renderer = new THREE.WebGLRenderer({ antialias:true });
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

// Luz
const light = new THREE.DirectionalLight(0xffffff, 1);
light.position.set(2,2,5);
scene.add(light);

// Cubo
const geometry = new THREE.BoxGeometry(1,1,1);
const material = new THREE.MeshStandardMaterial({ color:0x00ffcc });
const cube = new THREE.Mesh(geometry, material);
scene.add(cube);

camera.position.z = 3;

// WebSocket
const socket = new WebSocket('ws://' + location.hostname + ':81/');

socket.onmessage = (event) => {
  const data = JSON.parse(event.data);

  cube.rotation.x = data.roll * Math.PI / 180;
  cube.rotation.y = data.pitch * Math.PI / 180;
  cube.rotation.z = data.yaw * Math.PI / 180;
};

function animate(){
  requestAnimationFrame(animate);
  renderer.render(scene, camera);
}

animate();

window.addEventListener('resize', ()=>{
  camera.aspect = window.innerWidth/window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});

</script>
</body>
</html>
)rawliteral";

void setupMPU() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  accX = (Wire.read()<<8|Wire.read()) / 16384.0;
  accY = (Wire.read()<<8|Wire.read()) / 16384.0;
  accZ = (Wire.read()<<8|Wire.read()) / 16384.0;
  Wire.read(); Wire.read();
  gyroX = (Wire.read()<<8|Wire.read()) / 131.0;
  gyroY = (Wire.read()<<8|Wire.read()) / 131.0;
  gyroZ = (Wire.read()<<8|Wire.read()) / 131.0;
}

void calculateAngles() {
  float accRoll = atan2(accY, accZ) * 180 / PI;
  float accPitch = atan2(-accX, sqrt(accY*accY + accZ*accZ)) * 180 / PI;

  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0;
  prevTime = currentTime;

  roll = 0.96 * (roll + gyroX * dt) + 0.04 * accRoll;
  pitch = 0.96 * (pitch + gyroY * dt) + 0.04 * accPitch;
  yaw += gyroZ * dt; // yaw sin magnetómetro deriva
}

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void setup() {
  Serial.begin(115200);
  setupMPU();

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED);

  server.on("/", handleRoot);
  server.begin();

  webSocket.begin();

  prevTime = millis();
}

void loop() {
  server.handleClient();
  webSocket.loop();

  readMPU();
  calculateAngles();

  String json = "{\"roll\":" + String(roll) +
                ",\"pitch\":" + String(pitch) +
                ",\"yaw\":" + String(yaw) + "}";

  webSocket.broadcastTXT(json);
}