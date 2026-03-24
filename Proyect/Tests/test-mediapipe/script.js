import {
  PoseLandmarker,
  HandLandmarker,
  FilesetResolver,
  DrawingUtils
} from "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0";

const video = document.getElementById("webcam");
const canvasElement = document.getElementById("output_canvas");
const canvasCtx = canvasElement.getContext("2d");
const webcamButton = document.getElementById("webcamButton");

let poseLandmarker = undefined;
let handLandmarker = undefined;
let lastVideoTime = -1;

// --- CONFIGURACIÓN DE AJUSTES VISUALES ---
const Y_OFFSET = 0.03;      // Sube hombros y codos
const X_OFFSET = -0.018;      // Separa cadera
const X_OFFSET_P = -0.012;      // rodillas y pies hacia los lados

const COLORS = {
  leftArm: "#FF3D00", rightArm: "#00E676", torso: "#FFEE58",
  leftLeg: "#2979FF", rightLeg: "#D500F9", hands: "#00BCD4"
};

const POSE_PARTS = {
  leftArm: [11, 13, 15], rightArm: [12, 14, 16],
  torso: [11, 12, 23, 24], leftLeg: [23, 25, 27], rightLeg: [24, 26, 28]
};

const setupModels = async () => {
  const vision = await FilesetResolver.forVisionTasks(
    "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0/wasm"
  );

  poseLandmarker = await PoseLandmarker.createFromOptions(vision, {
    baseOptions: {
      modelAssetPath: `https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_heavy/float16/1/pose_landmarker_heavy.task`,
      delegate: "GPU"
    },
    runningMode: "VIDEO",
    numPoses: 1,
    minPoseDetectionConfidence: 0.7,
    minTrackingConfidence: 0.7
  });

  handLandmarker = await HandLandmarker.createFromOptions(vision, {
    baseOptions: {
      modelAssetPath: `https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task`,
      delegate: "GPU"
    },
    runningMode: "VIDEO",
    numHands: 2,
    minHandDetectionConfidence: 0.7,
    minHandPresenceConfidence: 0.7,
    minTrackingConfidence: 0.8
  });
};
setupModels();

function enableCam() {
  if (!poseLandmarker || !handLandmarker) return;
  webcamButton.style.display = "none";
  navigator.mediaDevices.getUserMedia({ video: { width: 1280, height: 720 } })
    .then((stream) => {
      video.srcObject = stream;
      video.addEventListener("loadeddata", predictWebcam);
    });
}

async function predictWebcam() {
  if (canvasElement.width !== video.videoWidth) {
    canvasElement.width = video.videoWidth;
    canvasElement.height = video.videoHeight;
  }

  let startTimeMs = performance.now();
  if (lastVideoTime !== video.currentTime) {
    lastVideoTime = video.currentTime;
    const [poseResult, handResult] = await Promise.all([
      poseLandmarker.detectForVideo(video, startTimeMs),
      handLandmarker.detectForVideo(video, startTimeMs)
    ]);
    canvasCtx.clearRect(0, 0, canvasElement.width, canvasElement.height);
    drawEverything(poseResult, handResult, canvasCtx);
  }
  window.requestAnimationFrame(predictWebcam);
}

function drawEverything(poseResult, handResult, ctx) {
  const drawingUtils = new DrawingUtils(ctx);
  const allowedIndices = [].concat(...Object.values(POSE_PARTS));

  if (poseResult.landmarks) {
    for (const rawLandmarks of poseResult.landmarks) {
      
      const adjustedLandmarks = rawLandmarks.map((lm, idx) => {
        let newPos = { ...lm };

        // 1. Subir Hombros y Codos
        if ([11, 12].includes(idx)) {
          newPos.y -= Y_OFFSET;
        }

        // 2. Separar Tren Inferior (Cadera, Rodillas, Tobillos)
        // Lado Izquierdo (23, 25, 27) se mueve a la izquierda (-X)
        if ([23].includes(idx)) {
          newPos.x -= X_OFFSET;
        }
        // Lado Derecho (24, 26, 28) se mueve a la derecha (+X)
        if ([24].includes(idx)) {
          newPos.x += X_OFFSET;
        }

        // 2. Separar Tren Inferior (Cadera, Rodillas, Tobillos)
        // Lado Izquierdo (23, 25, 27) se mueve a la izquierda (-X)
        if ([25, 27].includes(idx)) {
          newPos.x -= X_OFFSET_P;
        }
        // Lado Derecho (24, 26, 28) se mueve a la derecha (+X)
        if ([26, 28].includes(idx)) {
          newPos.x += X_OFFSET_P;
        }

        return newPos;
      });

      const filteredConnections = PoseLandmarker.POSE_CONNECTIONS.filter(conn => 
        allowedIndices.includes(conn.start) && allowedIndices.includes(conn.end)
      );

      drawingUtils.drawConnectors(adjustedLandmarks, filteredConnections, {
        color: "#E0E0E099",
        lineWidth: 4
      });

      for (const [partName, indices] of Object.entries(POSE_PARTS)) {
        const partLandmarks = indices.map(index => adjustedLandmarks[index]);
        drawingUtils.drawLandmarks(partLandmarks, {
          color: COLORS[partName],
          lineWidth: 2,
          radius: 5
        });
      }
    }
  }

  if (handResult.landmarks) {
    for (const landmarks of handResult.landmarks) {
      drawingUtils.drawConnectors(landmarks, HandLandmarker.HAND_CONNECTIONS, {
        color: COLORS.hands,
        lineWidth: 3
      });
      drawingUtils.drawLandmarks(landmarks, {
        color: "#FFFFFF",
        lineWidth: 1,
        radius: (data) => [4, 8, 12, 16, 20].includes(data.index) ? 4 : 2
      });
    }
  }
}

webcamButton.addEventListener("click", enableCam);