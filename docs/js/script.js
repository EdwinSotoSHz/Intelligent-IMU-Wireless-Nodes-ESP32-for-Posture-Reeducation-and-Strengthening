import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";

const viewer = document.getElementById("viewer");
const tagContainer = document.getElementById("tag-container");
const statusEl = document.getElementById("status");
const notePopup = document.getElementById("visor-note-popup");

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x111111);

const camera = new THREE.PerspectiveCamera(
    45, viewer.clientWidth / viewer.clientHeight, 0.01, 1000
);
camera.position.set(3, 2, 5);

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.setSize(viewer.clientWidth, viewer.clientHeight);
renderer.outputColorSpace = THREE.SRGBColorSpace;
viewer.appendChild(renderer.domElement);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.minDistance = 0.3;
controls.maxDistance = 50;

scene.add(new THREE.HemisphereLight(0xffffff, 0x444444, 2));
const dirLight = new THREE.DirectionalLight(0xffffff, 3);
dirLight.position.set(5, 10, 5);
scene.add(dirLight);

let allParts = [];
let modelGroup = null;
let currentHighlight = null;
const partNameMap = new Map();

/* ============ RUTA CORRECTA ============ */
const MODEL_URL = "./assets/PrototypeView.glb";
console.log("🔍 Cargando GLB desde:", MODEL_URL);

const loader = new GLTFLoader();
loader.load(
    MODEL_URL,
    (gltf) => {
        console.log("✅ GLB cargado");
        const model = gltf.scene;
        scene.add(model);
        modelGroup = model;

        const parts = [...model.children];
        allParts = parts;
        console.log(`📦 ${parts.length} piezas:`, parts.map(p => p.name));

        parts.forEach(part => {
            const name = part.name || "";
            if (name.includes("Case_Lid")) partNameMap.set(part, "Case_Lid (tapa)");
            else if (name.includes("Case_Body")) partNameMap.set(part, "Case_Body (base)");
            else partNameMap.set(part, name || "pieza");
        });

        statusEl.textContent = `${parts.length} piezas`;
        centerModel(model);
        buildTags(parts);

        const allTag = document.createElement("div");
        allTag.className = "visor-tag";
        allTag.textContent = "◉ Ver todo";
        tagContainer.appendChild(allTag);
        ["mouseenter", "click"].forEach(ev =>
            allTag.addEventListener(ev, e => { e.stopPropagation(); showAllParts(allTag); })
        );

        const withoutLidTag = document.createElement("div");
        withoutLidTag.className = "visor-tag";
        withoutLidTag.textContent = "◉ Sin Case_Lid";
        tagContainer.appendChild(withoutLidTag);
        ["mouseenter", "click"].forEach(ev =>
            withoutLidTag.addEventListener(ev, e => { e.stopPropagation(); showWithoutLid(withoutLidTag); })
        );
    },
    (progress) => {
        if (progress.total) {
            const pct = (progress.loaded / progress.total * 100).toFixed(0);
            statusEl.textContent = `cargando ${pct}%`;
        }
    },
    (err) => {
        console.error("❌ Error cargando GLB:", err);
        console.error("   URL intentada:", MODEL_URL);
        statusEl.textContent = "error al cargar modelo 3D";
    }
);

function centerModel(model) {
    const box = new THREE.Box3().setFromObject(model);
    const center = box.getCenter(new THREE.Vector3());
    const size = box.getSize(new THREE.Vector3());
    model.position.sub(center);
    const maxSize = Math.max(size.x, size.y, size.z);
    const dist = Math.max(maxSize * 2.8, 1.6);
    camera.position.set(dist * 0.9, dist * 0.7, dist * 1.2);
    controls.target.set(0, 0, 0);
    controls.update();
}

function buildTags(parts) {
    tagContainer.innerHTML = "";
    parts.forEach((part, index) => {
        const tag = document.createElement("div");
        tag.className = "visor-tag";
        const displayName = partNameMap.get(part) || part.name || `Pieza ${index + 1}`;
        tag.textContent = displayName;
        tag.dataset.index = index;

        tag.addEventListener("mouseenter", e => {
            e.stopPropagation();
            highlightPart(part, tag);
            showNote(displayName);
        });
        tag.addEventListener("mouseleave", hideNote);
        tag.addEventListener("click", e => {
            e.stopPropagation();
            highlightPart(part, tag);
            showNote(displayName);
        });

        tagContainer.appendChild(tag);
    });
}

function highlightPart(part, tagElement) {
    if (!allParts.length || currentHighlight === part) return;
    allParts.forEach(p => p.visible = false);
    part.visible = true;
    document.querySelectorAll(".visor-tag").forEach(t => t.classList.remove("active-tag"));
    tagElement?.classList.add("active-tag");
    focusObject(part);
    statusEl.textContent = `mostrando: ${partNameMap.get(part) || part.name || "pieza"}`;
    currentHighlight = part;
}

function showAllParts(tagElement) {
    if (!allParts.length || currentHighlight === "all") return;
    allParts.forEach(p => p.visible = true);
    document.querySelectorAll(".visor-tag").forEach(t => t.classList.remove("active-tag"));
    tagElement?.classList.add("active-tag");

    if (modelGroup) {
        const box = new THREE.Box3().setFromObject(modelGroup);
        const size = box.getSize(new THREE.Vector3());
        const maxSize = Math.max(size.x, size.y, size.z);
        const currentDist = camera.position.distanceTo(controls.target);
        const targetDist = Math.max(maxSize * 1.8, currentDist * 0.9);
        const dir = camera.position.clone().sub(controls.target).normalize();
        camera.position.copy(controls.target).add(dir.multiplyScalar(targetDist));
        controls.target.set(0, 0, 0);
        controls.update();
    }
    statusEl.textContent = `todas las piezas (${allParts.length})`;
    currentHighlight = "all";
    showNote("Mostrando todas las piezas");
}

function showWithoutLid(tagElement) {
    if (!allParts.length || currentHighlight === "withoutLid") return;
    allParts.forEach(p => p.visible = !(p.name || "").includes("Case_Lid"));
    document.querySelectorAll(".visor-tag").forEach(t => t.classList.remove("active-tag"));
    tagElement?.classList.add("active-tag");
    statusEl.textContent = "sin Case_Lid";
    currentHighlight = "withoutLid";
    showNote("Ocultando Case_Lid (tapa)");
}

function focusObject(object) {
    const box = new THREE.Box3().setFromObject(object);
    const center = box.getCenter(new THREE.Vector3());
    const size = box.getSize(new THREE.Vector3());
    const maxSize = Math.max(size.x, size.y, size.z);
    const dist = Math.max(maxSize * 2.2, 0.8);
    const dir = camera.position.clone().sub(controls.target).normalize();
    if (dir.length() < 0.001) dir.set(0.5, 0.3, 0.8).normalize();
    camera.position.copy(center).add(dir.multiplyScalar(dist));
    controls.target.copy(center);
    controls.update();
}

function showNote(text) { notePopup.textContent = text; notePopup.classList.add("visible"); }
function hideNote() { notePopup.classList.remove("visible"); }

const raycaster = new THREE.Raycaster();
const mouse = new THREE.Vector2();

renderer.domElement.addEventListener("mousemove", (event) => {
    const rect = renderer.domElement.getBoundingClientRect();
    mouse.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
    mouse.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;
    raycaster.setFromCamera(mouse, camera);

    const intersects = raycaster.intersectObjects(allParts, false);
    if (intersects.length > 0) {
        let hit = intersects[0].object;
        while (hit && !allParts.includes(hit)) hit = hit.parent;
        if (hit && allParts.includes(hit)) {
            showNote(partNameMap.get(hit) || hit.name || "pieza");
            renderer.domElement.style.cursor = "pointer";
            return;
        }
    }
    hideNote();
    renderer.domElement.style.cursor = "default";
});

renderer.domElement.addEventListener("mouseleave", () => {
    hideNote();
    renderer.domElement.style.cursor = "default";
});

window.addEventListener("resize", () => {
    const w = viewer.clientWidth;
    const h = viewer.clientHeight;
    if (!w || !h) return;
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
    renderer.setSize(w, h);
});

(function animate() {
    requestAnimationFrame(animate);
    controls.update();
    renderer.render(scene, camera);
})();