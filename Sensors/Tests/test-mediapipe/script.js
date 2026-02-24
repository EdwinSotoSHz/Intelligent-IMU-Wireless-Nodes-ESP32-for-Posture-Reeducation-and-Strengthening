import {
  PoseLandmarker,
  FilesetResolver,
  DrawingUtils
} from "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0";

const video = document.getElementById("webcam");
const canvasElement = document.getElementById("output_canvas");
const canvasCtx = canvasElement.getContext("2d");
const webcamButton = document.getElementById("webcamButton");
const imageButton = document.getElementById("imageButton");
const imageUpload = document.getElementById("imageUpload");
const imageContainer = document.getElementById("imageContainer");
const staticImage = document.getElementById("staticImage");
const imageOutputCanvas = document.getElementById("image_output_canvas");
const imageCanvasCtx = imageOutputCanvas.getContext("2d");

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

imageButton.addEventListener("click", () => {
  imageUpload.click();
});

imageUpload.addEventListener("change", (event) => {
  const file = event.target.files[0];
  if (file) {
    const reader = new FileReader();
    reader.onload = (e) => {
      staticImage.src = e.target.result;
      staticImage.onload = () => {
        // Ocultar el video y mostrar la imagen
        video.style.display = "none";
        canvasElement.style.display = "none";
        imageContainer.style.display = "block";
        
        // Ajustar tamaños
        imageOutputCanvas.width = staticImage.width;
        imageOutputCanvas.height = staticImage.height;
        
        // Procesar la imagen
        processImage(staticImage);
      };
    };
    reader.readAsDataURL(file);
  }
});

function enableCam() {
  if (!poseLandmarker) return;
  // Mostrar el video y ocultar la imagen
  video.style.display = "block";
  canvasElement.style.display = "block";
  imageContainer.style.display = "none";
  
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
          drawCustomLandmarks(landmark, canvasCtx);
        }
      }
    });
  }
  window.requestAnimationFrame(predictWebcam);
}

async function processImage(image) {
  if (!poseLandmarker) return;
  
  // Cambiar temporalmente el modo a IMAGE
  poseLandmarker.setOptions({ runningMode: "IMAGE" });
  
  // Detectar pose en la imagen
  const result = await poseLandmarker.detect(image);
  
  // Limpiar el canvas
  imageCanvasCtx.clearRect(0, 0, imageOutputCanvas.width, imageOutputCanvas.height);
  
  // Dibujar los landmarks
  if (result.landmarks) {
    for (const landmark of result.landmarks) {
      drawCustomLandmarks(landmark, imageCanvasCtx);
    }
  }
  
  // Volver al modo VIDEO
  poseLandmarker.setOptions({ runningMode: "VIDEO" });
}

// Función para pintar con colores personalizados
function drawCustomLandmarks(landmarks, ctx) {
  const drawingUtils = new DrawingUtils(ctx);

  // Dibujar cada parte con su color
  for (const [partName, indices] of Object.entries(POSE_PARTS)) {
    const partLandmarks = indices.map(index => landmarks[index]);
    
    // Dibujar los puntos
    drawingUtils.drawLandmarks(partLandmarks, {
      color: COLORS[partName],
      lineWidth: 2,
      radius: 2
    });
  }
  
  // Dibujar los conectores generales en gris para que resalten los colores de los puntos
  drawingUtils.drawConnectors(landmarks, PoseLandmarker.POSE_CONNECTIONS, {
    color: "#E0E0E0",
    lineWidth: 2
  });
}