// three.js
import * as THREE from 'https://unpkg.com/three@0.150.0/build/three.module.js';

// ============================
// VARIABLES CONTROLADAS
// ============================

let rollF = 0, pitchF = 0, yawF = 0;
let rollA = 0, pitchA = 0, yawA = 0;

// ==================== NUEVO: VARIABLES PARA INTERPOLACIÓN ====================
// Valores actuales (interpolados) que se usan en la animación
let currentRollA = 0, currentPitchA = 0, currentYawA = 0;
let currentRollF = 0, currentPitchF = 0, currentYawF = 0;

// Valores objetivo (los que llegan por WebSocket)
let targetRollA = 0, targetPitchA = 0, targetYawA = 0;
let targetRollF = 0, targetPitchF = 0, targetYawF = 0;

// Factor de interpolación (0.1 = suave, 0.3 = más rápido)
// Ajusta este valor entre 0.05 y 0.3 según prefieras
const LERP_FACTOR = 0.15;
// ============================================================================

// ============================
// ESCENA THREEJS
// ============================

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x111111);

const camera = new THREE.PerspectiveCamera(
    75,
    window.innerWidth / window.innerHeight,
    0.1,
    1000
);

camera.position.set(1, 1, 4);
camera.lookAt(1, 0, 0);

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

scene.add(new THREE.AmbientLight(0x606060));

const light = new THREE.DirectionalLight(0xffffff, 1);
light.position.set(5, 10, 7.5);
scene.add(light);

scene.add(new THREE.GridHelper(20, 40, 0x333333, 0x222222));
scene.add(new THREE.AxesHelper(2).setColors(0x00ff00, 0x0000ff, 0xff0000));

const width = 2, height = 0.5, depth = 1;


// ============================
// CUBO PADRE
// ============================

const geo1 = new THREE.BoxGeometry(width, height, depth);
geo1.translate(width / 2, 0, 0);

const mat1 = new THREE.MeshStandardMaterial({
    color: 0x00ffcc,
    transparent: true,
    opacity: 0.9
});

const cube1 = new THREE.Mesh(geo1, mat1);
scene.add(cube1);


// ============================
// CUBO HIJO
// ============================

const geo2 = new THREE.BoxGeometry(width, height, depth);
geo2.translate(width / 2, 0, 0);

const mat2 = new THREE.MeshStandardMaterial({
    color: 0xffcc00,
    transparent: true,
    opacity: 0.9
});

const cube2 = new THREE.Mesh(geo2, mat2);
cube2.position.x = width;
cube1.add(cube2);


// ============================
// MARCADORES
// ============================

scene.add(new THREE.Mesh(
    new THREE.SphereGeometry(0.05),
    new THREE.MeshBasicMaterial({ color: 0xff0000 })
));

const marker2 = new THREE.Mesh(
    new THREE.SphereGeometry(0.05),
    new THREE.MeshBasicMaterial({ color: 0xffffff })
);

cube2.add(marker2);


// ============================
// WEBSOCKET NODE-RED
// ============================

const socket = new WebSocket("ws://localhost:1880/imu");

socket.onmessage = (event) => {

    const data = JSON.parse(event.data);

    // ==================== MODIFICADO: Actualizar valores objetivo ====================
    // En lugar de actualizar directamente los valores que usa la animación,
    // actualizamos los valores objetivo para la interpolación
    targetRollA = data.rollA;
    targetPitchA = data.pitchA;
    targetYawA = data.yawA;

    targetRollF = data.rollF;
    targetPitchF = data.pitchF;
    targetYawF = data.yawF;

    // Mantenemos las variables originales por si son necesarias en otros lugares
    rollA = data.rollA;
    pitchA = data.pitchA;
    yawA = data.yawA;

    rollF = data.rollF;
    pitchF = data.pitchF;
    yawF = data.yawF;
    // ================================================================================
};


// ============================
// ANIMACION CON INTERPOLACIÓN
// ============================

function animate() {

    requestAnimationFrame(animate);

    // ==================== NUEVO: Interpolación lineal (lerp) ====================
    // Suaviza la transición entre el valor actual y el valor objetivo
    // La fórmula: current = current + (target - current) * factor
    currentRollA = currentRollA + (targetRollA - currentRollA) * LERP_FACTOR;
    currentPitchA = currentPitchA + (targetPitchA - currentPitchA) * LERP_FACTOR;
    currentYawA = currentYawA + (targetYawA - currentYawA) * LERP_FACTOR;

    currentRollF = currentRollF + (targetRollF - currentRollF) * LERP_FACTOR;
    currentPitchF = currentPitchF + (targetPitchF - currentPitchF) * LERP_FACTOR;
    currentYawF = currentYawF + (targetYawF - currentYawF) * LERP_FACTOR;
    // ============================================================================

    // Usamos los valores interpolados para la rotación
    // Padre
    cube1.rotation.x = currentPitchA * Math.PI / 180;
    cube1.rotation.y = currentYawA * Math.PI / 180;
    cube1.rotation.z = currentRollA * Math.PI / 180;

    // Hijo
    cube2.rotation.x = currentPitchF * Math.PI / 180;
    cube2.rotation.y = currentYawF * Math.PI / 180;
    cube2.rotation.z = currentRollF * Math.PI / 180;

    renderer.render(scene, camera);

}

animate();


window.addEventListener('resize', () => {

    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);

});