import * as THREE from "three";
import { OrbitControls } from "https://cdn.jsdelivr.net/npm/three@0.180.0/examples/jsm/controls/OrbitControls.js";
import { GLTFLoader } from "https://cdn.jsdelivr.net/npm/three@0.180.0/examples/jsm/loaders/GLTFLoader.js";

const viewer = document.getElementById("viewer");
const tagContainer = document.getElementById("tag-container");
const statusEl = document.getElementById("status");
const notePopup = document.getElementById("note-popup");

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x111111);

const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.01, 1000);
camera.position.set(3, 2, 5);

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.outputColorSpace = THREE.SRGBColorSpace;
viewer.appendChild(renderer.domElement);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.minDistance = 0.3;
controls.maxDistance = 50;

scene.add(new THREE.HemisphereLight(0xffffff, 0x444444, 2));
const light = new THREE.DirectionalLight(0xffffff, 3);
light.position.set(5, 10, 5);
scene.add(light);

let allParts = [];
let modelGroup = null;
let currentHighlight = null;
let partNameMap = new Map();

const loader = new GLTFLoader();
loader.load(
    "./assets/PrototypeView.glb",
    (gltf) => {
        const model = gltf.scene;
        scene.add(model);
        modelGroup = model;

        const parts = [...model.children];
        allParts = parts;

        parts.forEach(part => {
            const name = part.name || '';
            if (name.includes('Case_Lid')) {
                partNameMap.set(part, 'Case_Lid (tapa)');
            } else if (name.includes('Case_Body')) {
                partNameMap.set(part, 'Case_Body (base)');
            } else {
                partNameMap.set(part, name || 'pieza');
            }
        });

        statusEl.textContent = `${parts.length} piezas`;
        centerModel(model);
        buildTags(parts);

        const allTag = document.createElement("div");
        allTag.className = "tag";
        allTag.textContent = "◉ Ver todo";
        allTag.dataset.action = "all";
        tagContainer.appendChild(allTag);
        allTag.addEventListener("mouseenter", (e) => {
            e.stopPropagation();
            showAllParts(allTag);
        });
        allTag.addEventListener("click", (e) => {
            e.stopPropagation();
            showAllParts(allTag);
        });

        const withoutLidTag = document.createElement("div");
        withoutLidTag.className = "tag";
        withoutLidTag.textContent = "◉ Sin Case_Lid";
        withoutLidTag.dataset.action = "withoutLid";
        tagContainer.appendChild(withoutLidTag);
        withoutLidTag.addEventListener("mouseenter", (e) => {
            e.stopPropagation();
            showWithoutLid(withoutLidTag);
        });
        withoutLidTag.addEventListener("click", (e) => {
            e.stopPropagation();
            showWithoutLid(withoutLidTag);
        });

        if (parts.length === 0) statusEl.textContent = "sin piezas";
    },
    (progress) => {
        if (progress.total) {
            const pct = (progress.loaded / progress.total * 100).toFixed(0);
            statusEl.textContent = `cargando ${pct}%`;
        }
    },
    (error) => {
        console.error(error);
        statusEl.textContent = "error al cargar";
    }
);

function centerModel(model) {
    const box = new THREE.Box3().setFromObject(model);
    const center = box.getCenter(new THREE.Vector3());
    const size = box.getSize(new THREE.Vector3());
    model.position.sub(center);
    const maxSize = Math.max(size.x, size.y, size.z);
    const dist = Math.max(maxSize * 2.2, 1.2);
    camera.position.set(dist * 0.9, dist * 0.7, dist * 1.2);
    controls.target.set(0, 0, 0);
    controls.update();
}

function buildTags(parts) {
    tagContainer.innerHTML = '';
    parts.forEach((part, index) => {
        const tag = document.createElement("div");
        tag.className = "tag";
        const displayName = partNameMap.get(part) || part.name || `Pieza ${index + 1}`;
        tag.textContent = displayName;
        tag.dataset.index = index;

        tag.addEventListener("mouseenter", (e) => {
            e.stopPropagation();
            highlightPart(part, tag);
            showNote(displayName);
        });
        tag.addEventListener("mouseleave", () => {
            hideNote();
        });
        tag.addEventListener("click", (e) => {
            e.stopPropagation();
            highlightPart(part, tag);
            showNote(displayName);
        });

        tagContainer.appendChild(tag);
    });
}

function highlightPart(part, tagElement) {
    if (!allParts.length) return;
    if (currentHighlight === part) return;

    allParts.forEach(p => { p.visible = false; });
    part.visible = true;

    document.querySelectorAll('.tag').forEach(t => t.classList.remove('active-tag'));
    if (tagElement) tagElement.classList.add('active-tag');

    focusObject(part);
    statusEl.textContent = `mostrando: ${partNameMap.get(part) || part.name || 'pieza'}`;
    currentHighlight = part;
}

function showAllParts(tagElement) {
    if (!allParts.length) return;
    if (currentHighlight === 'all') return;

    allParts.forEach(p => { p.visible = true; });
    document.querySelectorAll('.tag').forEach(t => t.classList.remove('active-tag'));
    if (tagElement) tagElement.classList.add('active-tag');

    // Restaurar vista general sin reiniciar zoom
    if (modelGroup) {
        const box = new THREE.Box3().setFromObject(modelGroup);
        const center = box.getCenter(new THREE.Vector3());
        const size = box.getSize(new THREE.Vector3());
        const maxSize = Math.max(size.x, size.y, size.z);
        const currentDist = camera.position.distanceTo(controls.target);
        const targetDist = Math.max(maxSize * 1.8, currentDist * 0.9);
        const dir = new THREE.Vector3().copy(camera.position).sub(controls.target).normalize();
        const newPos = new THREE.Vector3().copy(controls.target).add(dir.multiplyScalar(targetDist));
        camera.position.copy(newPos);
        controls.target.set(0, 0, 0);
        controls.update();

        statusEl.textContent = `todas las piezas (${allParts.length})`;
        currentHighlight = 'all';
        showNote('Mostrando todas las piezas');
    }
}

function showWithoutLid(tagElement) {
    if (!allParts.length) return;
    if (currentHighlight === 'withoutLid') return;

    allParts.forEach(p => {
        const name = p.name || '';
        if (name.includes('Case_Lid')) {
            p.visible = false;
        } else {
            p.visible = true;
        }
    });

    document.querySelectorAll('.tag').forEach(t => t.classList.remove('active-tag'));
    if (tagElement) tagElement.classList.add('active-tag');

    if (modelGroup) {
        const visibleParts = allParts.filter(p => p.visible);
        if (visibleParts.length) {
            const tempGroup = new THREE.Group();
            visibleParts.forEach(p => tempGroup.add(p.clone()));
            const box = new THREE.Box3().setFromObject(tempGroup);
            const center = box.getCenter(new THREE.Vector3());
            const size = box.getSize(new THREE.Vector3());
            const maxSize = Math.max(size.x, size.y, size.z);
            const currentDist = camera.position.distanceTo(controls.target);
            const targetDist = Math.max(maxSize * 1.8, currentDist * 0.9);
            const dir = new THREE.Vector3().copy(camera.position).sub(controls.target).normalize();
            const newPos = new THREE.Vector3().copy(center).add(dir.multiplyScalar(targetDist));
            camera.position.copy(newPos);
            controls.target.copy(center);
            controls.update();
        }
    }

    statusEl.textContent = 'sin Case_Lid';
    currentHighlight = 'withoutLid';
    showNote('Ocultando Case_Lid (tapa)');
}

function focusObject(object) {
    const box = new THREE.Box3().setFromObject(object);
    const center = box.getCenter(new THREE.Vector3());
    const size = box.getSize(new THREE.Vector3());
    const maxSize = Math.max(size.x, size.y, size.z);
    const dist = Math.max(maxSize * 2.2, 0.8);
    const dir = new THREE.Vector3().copy(camera.position).sub(controls.target).normalize();
    if (dir.length() < 0.001) dir.set(0.5, 0.3, 0.8).normalize();
    const newPos = new THREE.Vector3().copy(center).add(dir.multiplyScalar(dist));
    camera.position.copy(newPos);
    controls.target.copy(center);
    controls.update();
}

function showNote(text) {
    notePopup.textContent = text;
    notePopup.classList.add('visible');
}

function hideNote() {
    notePopup.classList.remove('visible');
}

const raycaster = new THREE.Raycaster();
const mouse = new THREE.Vector2();
let hoveredObject = null;

renderer.domElement.addEventListener('mousemove', (event) => {
    const rect = renderer.domElement.getBoundingClientRect();
    mouse.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
    mouse.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;

    raycaster.setFromCamera(mouse, camera);

    const intersects = raycaster.intersectObjects(allParts, false);

    if (intersects.length > 0) {
        let hitObject = intersects[0].object;
        while (hitObject && !allParts.includes(hitObject)) {
            hitObject = hitObject.parent;
        }
        if (hitObject && allParts.includes(hitObject)) {
            const displayName = partNameMap.get(hitObject) || hitObject.name || 'pieza';
            showNote(displayName);
            hoveredObject = hitObject;
            renderer.domElement.style.cursor = 'pointer';
            return;
        }
    }
    if (!event.target.closest('.tag')) {
        hideNote();
        hoveredObject = null;
    }
});

renderer.domElement.addEventListener('mouseleave', () => {
    hideNote();
    renderer.domElement.style.cursor = 'default';
});

window.addEventListener("resize", () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
});

function animate() {
    requestAnimationFrame(animate);
    controls.update();
    renderer.render(scene, camera);
}
animate();

setTimeout(() => {
    if (allParts.length === 0 && !modelGroup) {
        statusEl.textContent = "modelo no encontrado";
    }
}, 3000);