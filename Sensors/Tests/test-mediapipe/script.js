import {
  PoseLandmarker,
  FilesetResolver,
  DrawingUtils
} from "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0";

const video = document.getElementById("webcam");
const canvasElement = document.getElementById("output_canvas");
const canvasCtx = canvasElement.getContext("2d");
const webcamButton = document.getElementById("webcamButton");

let poseLandmarker = undefined;
let lastVideoTime = -1;

// Definición de colores por extremidad
const COLORS = {
  face: "#FFFFFF",        // Blanco
  leftArm: "#FF3D00",     // Naranja/Rojo
  rightArm: "#00E676",    // Verde
  torso: "#FFEE58",       // Amarillo
  leftLeg: "#2979FF",     // Azul
  rightLeg: "#D500F9"     // Morado
};

// Índices de MediaPipe para cada parte
const POSE_PARTS = {
  face: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
  leftArm: [11, 13, 15, 17, 19, 21],
  rightArm: [12, 14, 16, 18, 20, 22],
  torso: [11, 12, 23, 24],
  leftLeg: [23, 25, 27, 29, 31],
  rightLeg: [24, 26, 28, 30, 32]
};

const createPoseLandmarker = async () => {
  const vision = await FilesetResolver.forVisionTasks(
    "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0/wasm"
  );
  poseLandmarker = await PoseLandmarker.createFromOptions(vision, {
    baseOptions: {
      modelAssetPath: `https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_lite/float16/1/pose_landmarker_lite.task`,
      delegate: "GPU"
    },
    runningMode: "VIDEO"
  });
};
createPoseLandmarker();

if (navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
  webcamButton.addEventListener("click", enableCam);
}

function enableCam() {
  if (!poseLandmarker) return;
  webcamButton.classList.add("removed");
  const constraints = { video: { width: 1280, height: 720 } };
  navigator.mediaDevices.getUserMedia(constraints).then((stream) => {
    video.srcObject = stream;
    video.addEventListener("loadeddata", predictWebcam);
  });
}

async function predictWebcam() {
  canvasElement.style.height = video.videoHeight;
  canvasElement.style.width = video.videoWidth;

  let startTimeMs = performance.now();
  if (lastVideoTime !== video.currentTime) {
    lastVideoTime = video.currentTime;
    poseLandmarker.detectForVideo(video, startTimeMs, (result) => {
      canvasCtx.clearRect(0, 0, canvasElement.width, canvasElement.height);
      
      if (result.landmarks) {
        for (const landmark of result.landmarks) {
          drawCustomLandmarks(landmark);
        }
      }
    });
  }
  window.requestAnimationFrame(predictWebcam);
}

// Función para pintar con colores personalizados
function drawCustomLandmarks(landmarks) {
  const drawingUtils = new DrawingUtils(canvasCtx);

  // Dibujar cada parte con su color
  for (const [partName, indices] of Object.entries(POSE_PARTS)) {
    const partLandmarks = indices.map(index => landmarks[index]);
    
    // Dibujar los puntos
    drawingUtils.drawLandmarks(partLandmarks, {
      color: COLORS[partName],
      lineWidth: 2,
      radius: 2
    });

    // Dibujar conexiones básicas para esa parte
    // Nota: Para conexiones complejas entre puntos se requiere la lista de conexiones,
    // aquí simplificamos dibujando los puntos destacados.
  }
  
  // Dibujar los conectores generales en gris para que resalten los colores de los puntos
  drawingUtils.drawConnectors(landmarks, PoseLandmarker.POSE_CONNECTIONS, {
    color: "#E0E0E0",
    lineWidth: 2
  });
}