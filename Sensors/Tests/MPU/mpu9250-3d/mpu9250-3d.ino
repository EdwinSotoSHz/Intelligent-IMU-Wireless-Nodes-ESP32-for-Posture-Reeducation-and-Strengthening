#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>

#define SDA_PIN 32
#define SCL_PIN 33
#define MPU_ADDR 0x68

#define ACCEL_SCALE 16384.0
#define GYRO_SCALE 131.0

// ==== TUS BIAS MEDIDOS (Ajustados) ====
#define GYRO_BIAS_X  -554.73
#define GYRO_BIAS_Y   483.33
#define GYRO_BIAS_Z     5.64

#define ACC_BIAS_X  (644.0)   // Usa los valores crudos que mediste antes
#define ACC_BIAS_Y  (-760.0)
#define ACC_BIAS_Z  (18048.0 - 16384.0) 
// ===========================

const char* ssid = "AsusE";
const char* password = "23011edpi";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

int16_t axRaw, ayRaw, azRaw, gxRaw, gyRaw, gzRaw;
float roll = 0, pitch = 0, yaw = 0;
unsigned long prevTime;
unsigned long lastSend = 0;

// HTML/JS con corrección de ejes para Three.js
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>MPU 3D Pro</title>
    <style>
        body{margin:0; overflow:hidden; background:#111; color:#0ff; font-family:sans-serif}
        #info{position:absolute; top:10px; left:10px; pointer-events:none; background:rgba(0,0,0,0.5); padding:10px; border-radius:5px}
    </style>
</head>
<body>
<div id="info">
    Roll: <span id="r">0</span>° | Pitch: <span id="p">0</span>° | Yaw: <span id="y">0</span>°
</div>
<script type="module">
    import * as THREE from 'https://unpkg.com/three@0.150.0/build/three.module.js';

    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(75, window.innerWidth/window.innerHeight, 0.1, 100);
    const renderer = new THREE.WebGLRenderer({antialias:true});
    renderer.setSize(window.innerWidth, window.innerHeight);
    document.body.appendChild(renderer.domElement);

    // Luces y Suelo para referencia
    scene.add(new THREE.GridHelper(10, 10));
    const light = new THREE.DirectionalLight(0xffffff, 1);
    light.position.set(5, 5, 5);
    scene.add(light);
    scene.add(new THREE.AmbientLight(0x404040));

    // El objeto (Caja)
    const geometry = new THREE.BoxGeometry(3, 0.5, 1); 
    const material = new THREE.MeshStandardMaterial({color: 0x00ffcc});
    const cube = new THREE.Mesh(geometry, material);
    scene.add(cube);

    camera.position.set(0, 2, -4);
    camera.lookAt(0, 0, 0);

    const socket = new WebSocket('ws://'+location.hostname+':81/');
    socket.onmessage = (event) => {
        const d = JSON.parse(event.data);
        cube.rotation.x = d.pitch * Math.PI/180;
        cube.rotation.z = -d.roll * Math.PI/180;
        cube.rotation.y = -d.yaw * Math.PI/180;

        document.getElementById("r").textContent = d.roll.toFixed(1);
        document.getElementById("p").textContent = d.pitch.toFixed(1);
        document.getElementById("y").textContent = d.yaw.toFixed(1);
    };

    function animate() {
        requestAnimationFrame(animate);
        renderer.render(scene, camera);
    }
    animate();

    window.onresize = () => {
        camera.aspect = window.innerWidth/window.innerHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(window.innerWidth, window.innerHeight);
    };
</script>
</body>
</html>
)rawliteral";

void readMPU() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 14, true);
    axRaw = Wire.read()<<8|Wire.read();
    ayRaw = Wire.read()<<8|Wire.read();
    azRaw = Wire.read()<<8|Wire.read();
    Wire.read(); Wire.read();
    gxRaw = Wire.read()<<8|Wire.read();
    gyRaw = Wire.read()<<8|Wire.read();
    gzRaw = Wire.read()<<8|Wire.read();
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // MPU Init
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); Wire.write(0x00); Wire.endTransmission();

    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) delay(500);

    server.on("/", [](){ server.send_P(200, "text/html", index_html); });
    server.begin();
    webSocket.begin();

    // Sincronización inicial
    readMPU();
    float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
    float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
    float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;
    
    roll = atan2(ay, az) * 180 / PI;
    pitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;
    prevTime = micros();
}

void loop() {
    server.handleClient();
    webSocket.loop();

    unsigned long now = micros();
    float dt = (now - prevTime) / 1000000.0;
    prevTime = now;

    readMPU();

    // Convertir a unidades físicas corregidas
    float gx = (gxRaw - GYRO_BIAS_X) / GYRO_SCALE;
    float gy = (gyRaw - GYRO_BIAS_Y) / GYRO_SCALE;
    float gz = (gzRaw - GYRO_BIAS_Z) / GYRO_SCALE;

    float ax = (axRaw - ACC_BIAS_X) / ACCEL_SCALE;
    float ay = (ayRaw - ACC_BIAS_Y) / ACCEL_SCALE;
    float az = (azRaw - ACC_BIAS_Z) / ACCEL_SCALE;

    // Acelerómetro (Referencia de gravedad)
    float accRoll = atan2(ay, az) * 180 / PI;
    float accPitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180 / PI;

    // Filtro Complementario (Funde la estabilidad del Acc con la rapidez del Gyro)
    roll = 0.96 * (roll + gx * dt) + 0.04 * accRoll;
    pitch = 0.96 * (pitch + gy * dt) + 0.04 * accPitch;
    yaw += gz * dt; // El Yaw solo se puede medir con Gyro o Magnetómetro

    // Enviar datos cada 30ms (aprox 33fps) para no saturar
    if (millis() - lastSend > 30) {
        String json = "{\"roll\":" + String(roll) + 
                      ",\"pitch\":" + String(pitch) + 
                      ",\"yaw\":" + String(yaw) + "}";
        webSocket.broadcastTXT(json);
        lastSend = millis();
    }
}