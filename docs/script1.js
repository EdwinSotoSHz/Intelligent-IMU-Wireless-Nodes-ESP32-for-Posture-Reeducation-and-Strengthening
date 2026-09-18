import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";

// --- Referencias ---
const canvas = document.getElementById('exvCanvas');
const labelEl = document.getElementById('exvLabel');
const introEl = document.querySelector('.exv-intro');
const currentPartEl = document.getElementById('currentPart');
const progressFill = document.getElementById('progressFill');
const partsListItems = document.querySelectorAll('#partsList li');

// --- Renderer ---
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.outputColorSpace = THREE.SRGBColorSpace;

// --- Escena ---
const scene = new THREE.Scene();
scene.background = new THREE.Color(0x0a0e14);

// --- Cámara ---
const camera = new THREE.PerspectiveCamera(45, 1, 0.1, 200);

// --- Luces ---
scene.add(new THREE.AmbientLight(0xffffff, 0.55));
const key = new THREE.DirectionalLight(0xffffff, 1.0);
key.position.set(10, 14, 8);
scene.add(key);
const rim = new THREE.DirectionalLight(0x38bdf8, 0.5);
rim.position.set(-8, -4, -10);
scene.add(rim);

// --- Grupo del modelo ---
const group = new THREE.Group();
scene.add(group);

// --- Estado ---
let parts = [];
let loaded = false;
let cameraDistance = 20;
const clock = new THREE.Clock();

// --- Orden de piezas ---
const ORDER = [
    "ESP32_C3",
    "Shield_ESP32_C3",
    "AD8232",
    "MPU9250",
    "LiPo_Battery",
    "PCB",
    "Case_Lid",
    "Case_Body"
];

// --- Utilitarios ---
const box = (object) => new THREE.Box3().setFromObject(object);
const center = (object) => box(object).getCenter(new THREE.Vector3());
const size = (object) => box(object).getSize(new THREE.Vector3());

// --- Cargar GLB ---
const loader = new GLTFLoader();
loader.load(
    './assets/PrototypeView.glb',

    (gltf) => {
        const model = gltf.scene;
        group.add(model);

        // Centrar modelo
        model.position.sub(
            box(model).getCenter(new THREE.Vector3())
        );

        model.rotation.y = Math.PI * -0.6;

        const children = [...model.children];

        ORDER.forEach((name, i) => {
            const object = children.find(o => o.name?.includes(name)) || children[i];
            if (!object) return;

            parts.push({
                object,
                name,
                initial: object.position.clone(),
                center: center(object),
                size: size(object),
                target: new THREE.Vector3(),
                // Guardar el centro absoluto para la fase de enfoque
                worldCenter: new THREE.Vector3()
            });
        });

        // Calcular posiciones explosionadas
        const SPACING = 6;
        const COLUMN_ANGLE = THREE.MathUtils.degToRad(10);

        let y = parts.reduce(
            (total, p) => total + p.size.y + SPACING,
            -SPACING
        ) / 2;

        parts.forEach((p) => {
            const offset = p.center.y - p.initial.y;

            p.target.set(
                Math.sin(COLUMN_ANGLE) * (y - offset),
                y - offset,
                0
            );

            // Guardar el centro absoluto (posición inicial + centro local)
            p.worldCenter.copy(p.initial);
            p.worldCenter.y += p.center.y;

            y -= p.size.y + SPACING;
        });

        // Ajustar cámara
        const modelSize = size(model);
        cameraDistance = Math.max(...modelSize.toArray()) * 2.2;

        camera.position.set(
            cameraDistance * 0.6,
            cameraDistance * 0.25,
            cameraDistance * 0.6
        );
        camera.lookAt(0, 0, 0);

        loaded = true;
        labelEl.textContent = 'ENSAMBLADO';
        if (currentPartEl) currentPartEl.textContent = 'ENSAMBLADO';

        if (partsListItems.length) {
            partsListItems[0].classList.add('active');
        }
    },

    (progress) => {
        if (progress.total) {
            const percent = Math.round((progress.loaded / progress.total) * 100);
            labelEl.textContent = `${percent}%`;
        }
    },

    (error) => {
        console.error(error);
        labelEl.textContent = "Error";
    }
);

// --- GSAP + ScrollTrigger ---
gsap.registerPlugin(ScrollTrigger);

const ex = { p: 0 };

// Scroll muy largo para las 3 fases
gsap.to(ex, {
    p: 1,
    ease: 'none',
    scrollTrigger: {
        trigger: '#content',
        start: 'top top',
        end: '+=600%', // Scroll más largo para las 3 fases
        scrub: 0.6,
        invalidateOnRefresh: true
    }
});

// --- Funciones de easing ---
function easeInOut(x) {
    return x < 0.5 ? 4 * x * x * x : 1 - Math.pow(-2 * x + 2, 3) / 2;
}

function easeOut(x) {
    return 1 - Math.pow(1 - x, 3);
}

// --- Resize ---
function resize() {
    const w = canvas.clientWidth;
    const h = canvas.clientHeight;

    const isMobile = window.innerWidth <= 768;
    const width = isMobile ? window.innerWidth : window.innerWidth * 0.5;

    renderer.setSize(width, window.innerHeight, false);
    camera.aspect = width / window.innerHeight;
    camera.updateProjectionMatrix();
}

// --- Actualizar UI ---
function updateUI(explode, focusIndex) {
    if (explode < 0.02) {
        labelEl.textContent = 'ENSAMBLADO';
        if (currentPartEl) currentPartEl.textContent = 'ENSAMBLADO';
    } else if (explode > 0.97 && focusIndex === parts.length - 1) {
        labelEl.textContent = `• ${parts[focusIndex]?.name || 'PIEZA'}`;
        if (currentPartEl) currentPartEl.textContent = parts[focusIndex]?.name || 'PIEZA';
    } else {
        const index = Math.min(parts.length - 1, Math.floor(explode * parts.length));
        const partName = parts[index]?.name || 'PIEZA';
        labelEl.textContent = partName;
        if (currentPartEl) currentPartEl.textContent = partName;

        partsListItems.forEach((li, i) => {
            li.classList.toggle('active', i === index);
        });
    }

    if (progressFill) {
        progressFill.style.width = `${explode * 100}%`;
    }
}

// --- Animación ---
function animate() {
    requestAnimationFrame(animate);

    const t = clock.getElapsedTime();
    const p = ex.p;

    if (introEl) introEl.style.opacity = p > 0.03 ? '0' : '1';

    if (!loaded || parts.length === 0) {
        renderer.render(scene, camera);
        return;
    }

    // --- TRES FASES ---
    // Fase 1: 0-0.40 → Explosión
    // Fase 2: 0.40-0.80 → Enfoque pieza por pieza (subiendo)
    // Fase 3: 0.80-1.00 → Última pieza centrada + órbita

    const phase1 = Math.min(1, p / 0.40); // Explosión
    const phase2 = Math.max(0, Math.min(1, (p - 0.40) / 0.40)); // Enfoque
    const phase3 = Math.max(0, (p - 0.80) / 0.20); // Órbita final

    const explode = easeInOut(phase1);

    // --- 1. MOVER PIEZAS (Explosión) ---
    parts.forEach((part, i) => {
        const delay = (parts.length - 1 - i) * 0.08;
        const local = Math.min(1, Math.max(0, (explode - delay) / 0.7));

        part.object.position.lerpVectors(
            part.initial,
            part.target,
            local
        );

        const rotSpeed = 0.4;
        part.object.rotation.y = Math.sin(t * rotSpeed + i) * 0.015 * local;
    });

    // --- 2. CÁLCULO DE ÍNDICE DE ENFOQUE ---
    // El enfoque avanza de 0 a N-1 durante la fase 2
    const focusProgress = easeOut(phase2);
    const focusIndex = Math.min(
        parts.length - 1,
        Math.floor(focusProgress * (parts.length - 1))
    );

    // Interpolación suave entre piezas
    const nextIndex = Math.min(focusIndex + 1, parts.length - 1);
    const focusT = (focusProgress * (parts.length - 1)) - focusIndex;

    // Centro actual (interpolado entre pieza actual y siguiente)
    const currentCenter = new THREE.Vector3();
    const nextCenter = new THREE.Vector3();

    if (parts[focusIndex]) {
        currentCenter.copy(parts[focusIndex].worldCenter);
    }
    if (parts[nextIndex]) {
        nextCenter.copy(parts[nextIndex].worldCenter);
    }

    const targetCenter = new THREE.Vector3().lerpVectors(
        currentCenter,
        nextCenter,
        focusT
    );

    // --- 3. POSICIÓN DE CÁMARA ---
    // Fase 1: Cámara fija viendo el centro del modelo
    // Fase 2: Cámara se acerca a cada pieza
    // Fase 3: Cámara orbita alrededor de la última pieza

    const orbitAngle = -0.5 + phase3 * 2.6;
    const orbitRadius = cameraDistance * 0.7;

    // La cámara se acerca gradualmente durante la fase 2
    const zoomFactor = 1 - phase2 * 0.5; // 1 → 0.5
    const finalRadius = orbitRadius * (0.5 + zoomFactor * 0.5);

    // Altura: baja hasta el nivel de la pieza
    const heightOffset = 7 * (1 - phase2 * 0.6) + phase3 * 2;
    const targetHeight = targetCenter.y + heightOffset + Math.sin(t * 0.3) * 0.4;

    // Posición final de la cámara
    let targetX, targetZ;

    if (phase3 > 0) {
        // Fase 3: Órbita alrededor de la última pieza
        targetX = targetCenter.x + Math.sin(orbitAngle) * finalRadius * 0.8;
        targetZ = targetCenter.z + Math.cos(orbitAngle) * finalRadius * 0.8;
    } else {
        // Fase 1 y 2: Cámara en posición fija pero siguiendo el centro
        const baseAngle = -0.5;
        const baseRadius = cameraDistance * 0.9;
        targetX = targetCenter.x + Math.sin(baseAngle) * baseRadius * (0.7 + 0.3 * (1 - phase2));
        targetZ = targetCenter.z + Math.cos(baseAngle) * baseRadius * (0.7 + 0.3 * (1 - phase2));
    }

    // Movimiento suave de cámara
    camera.position.x += (targetX - camera.position.x) * 0.06;
    camera.position.z += (targetZ - camera.position.z) * 0.06;
    camera.position.y += (targetHeight - camera.position.y) * 0.06;

    // Mirar al centro objetivo
    const lookTarget = new THREE.Vector3(
        targetCenter.x,
        targetCenter.y + 0.5,
        targetCenter.z
    );
    camera.lookAt(lookTarget);

    // --- 4. ROTACIÓN DEL MODELO ---
    // Se detiene cuando empezamos a enfocar piezas individuales
    if (phase2 < 0.1) {
        group.rotation.y = t * 0.05;
    } else {
        // Rotación muy lenta o detenida
        group.rotation.y += 0.0002;
    }

    // --- 5. UI ---
    updateUI(explode, focusIndex);

    renderer.render(scene, camera);
}

// --- Event listeners ---
window.addEventListener('resize', resize);

// --- Botón reset ---
document.getElementById('resetView')?.addEventListener('click', () => {
    window.scrollTo({ top: 0, behavior: 'smooth' });
});

// --- Iniciar ---
resize();
animate();

console.log('🚀 Efecto 3 fases iniciado');
console.log(`📦 ${parts.length} piezas cargadas`);