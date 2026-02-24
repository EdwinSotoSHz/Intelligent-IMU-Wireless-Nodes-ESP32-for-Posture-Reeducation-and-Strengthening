import {
  FilesetResolver,
  PoseLandmarker,
  DrawingUtils
} from "https://cdn.skypack.dev/@mediapipe/tasks-vision@0.10.0";

const videoElement = document.getElementById('webcam');
const canvasElement = document.getElementById('output_canvas');
const ctx = canvasElement.getContext('2d');
const startButton = document.getElementById('startButton');

let poseLandmarker;
let isDetecting = false;

// Colores para cada landmark
const landmarkColors = [
  "#e6194b","#3cb44b","#ffe119","#4363d8","#f58231","#911eb4","#46f0f0",
  "#f032e6","#bcf60c","#fabebe","#008080","#e6beff","#9a6324","#fffac8",
  "#800000","#aaffc3","#808000","#ffd8b1","#000075","#808080","#ffffff",
  "#000000","#ffe4e1","#8b0000","#00ff00","#0000ff","#ff00ff","#ffff00",
  "#00ffff","#ff7f00","#7f00ff","#7fff00","#007fff"
];

async function initPose() {
  const vision = await FilesetResolver.forVisionTasks(
    "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0/wasm"
  );

  poseLandmarker = await PoseLandmarker.createFromOptions(vision, {
    baseOptions: {
      modelAssetPath: `https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_lite/float16/1/pose_landmarker_lite.task`,
      delegate: "GPU"  // Usa GPU si está disponible
    },
    runningMode: "VIDEO",
    numPoses: 1,
  });
}

// Dibuja los landmarks de colores
function drawLandmarks(results) {
  ctx.save();
  ctx.clearRect(0, 0, canvasElement.width, canvasElement.height);
  ctx.drawImage(videoElement, 0, 0, canvasElement.width, canvasElement.height);

  if (!results || !results.landmarks) {
    ctx.restore();
    return;
  }

  const landmarks = results.landmarks[0] ?? [];

  for (let i = 0; i < landmarks.length; i++) {
    const lm = landmarks[i];
    const x = lm.x * canvasElement.width;
    const y = lm.y * canvasElement.height;
    const col = landmarkColors[i % landmarkColors.length];

    ctx.fillStyle = col;
    ctx.beginPath();
    ctx.arc(x, y, 6, 0, 2 * Math.PI);
    ctx.fill();
  }

  ctx.restore();
}

// Bucle de detección
async function detectFrame() {
  if (!isDetecting) return;

  poseLandmarker.detectForVideo(videoElement, performance.now(), (results) => {
    drawLandmarks(results);
    requestAnimationFrame(detectFrame);
  });
}

// Iniciar cámara + detección
async function startCamera() {
  const stream = await navigator.mediaDevices.getUserMedia({ video: true });
  videoElement.srcObject = stream;

  videoElement.onloadeddata = () => {
    isDetecting = true;
    detectFrame();
  };
}

startButton.addEventListener("click", async () => {
  if (!poseLandmarker) {
    await initPose();
  }
  startCamera();
});