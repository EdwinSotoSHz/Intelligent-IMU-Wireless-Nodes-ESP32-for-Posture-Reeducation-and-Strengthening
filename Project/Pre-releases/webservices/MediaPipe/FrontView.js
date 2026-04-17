import {
  PoseLandmarker,
  HandLandmarker,
  FaceLandmarker,
  FilesetResolver,
  DrawingUtils
} from "../node_modules/@mediapipe/tasks-vision/vision_bundle.mjs";
//! Opción web CDN
//! from "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0";
import { AngleDrawer } from "./AngleDrawer.js";

const video = document.getElementById("webcam");
const canvasElement = document.getElementById("output_canvas");
const canvasCtx = canvasElement.getContext("2d");
const webcamButton = document.getElementById("webcamButton");

let poseLandmarker = undefined;
let handLandmarker = undefined;
let faceLandmarker = undefined;
let lastVideoTime = -1;

// --- CONFIGURACIÓN DE AJUSTES VISUALES ---
const Y_OFFSET = 0.025;
const X_OFFSET = -0.018;
const X_OFFSET_P = -0.012;

const COLORS = {
  leftArm: "#FF3D00", rightArm: "#00E676", torso: "#FFEE58",
  leftLeg: "#2979FF", rightLeg: "#D500F9", hands: "#00BCD4",
  face: "#FFFFFF"
};

const POSE_PARTS = {
  leftArm: [11, 13, 15], rightArm: [12, 14, 16],
  torso: [11, 12, 23, 24], leftLeg: [23, 25, 27], rightLeg: [24, 26, 28]
};

const setupModels = async () => {
/*
! Opción web CDN
!  const vision = await FilesetResolver.forVisionTasks(
!    "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0/wasm"
!  );
*/
  const vision = await FilesetResolver.forVisionTasks("../MediaPipe/wasm");

  poseLandmarker = await PoseLandmarker.createFromOptions(vision, {
    baseOptions: {
      //! Opción web CDN
      //! modelAssetPath: `https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_heavy/float16/1/pose_landmarker_heavy.task`,
      modelAssetPath: "../MediaPipe/models/pose_landmarker_heavy.task",
      delegate: "GPU"
    },
    runningMode: "VIDEO",
    numPoses: 1,
    minPoseDetectionConfidence: 0.7,
    minTrackingConfidence: 0.8
  });

  handLandmarker = await HandLandmarker.createFromOptions(vision, {
    baseOptions: {
      //! Opción web CDN
      //! modelAssetPath: `https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task`,
      modelAssetPath: "../MediaPipe/models/hand_landmarker.task",
      delegate: "GPU"
    },
    runningMode: "VIDEO",
    numHands: 2,
    minHandDetectionConfidence: 0.7,
    minHandPresenceConfidence: 0.8,
    minTrackingConfidence: 0.65
  });

  // solo contorno
  faceLandmarker = await FaceLandmarker.createFromOptions(vision, {
    baseOptions: {
      //! Opción web CDN
      //! modelAssetPath: `https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task`,
      modelAssetPath: "../MediaPipe/models/face_landmarker.task",
      delegate: "GPU"
    },
    runningMode: "VIDEO",
    minFaceDetectionConfidence: 0.2,
    minTrackingConfidence: 0.3,
    numFaces: 1
  });
};

setupModels();

let currentCameraIndex = 0; // * CAM
async function enableCam() {
  if (!poseLandmarker || !handLandmarker || !faceLandmarker) return;

  webcamButton.style.display = "none";
  z
  await navigator.mediaDevices.getUserMedia({ video: true });
  const devices = await navigator.mediaDevices.enumerateDevices();
  const videoDevices = devices.filter(d => d.kind === "videoinput");

  console.log("Cámaras disponibles:");
  videoDevices.forEach((cam, i) => {
    console.log(`${i} → ${cam.label}`);
  });

  const selectedCamera = videoDevices[currentCameraIndex];

  if (!selectedCamera) {
    console.error("Índice inválido");
    return;
  }

  // * IMPORTANTE: detener cámara anterior
  if (video.srcObject) {
    video.srcObject.getTracks().forEach(track => track.stop());
  }

  const constraints = {
    video: {
      deviceId: { exact: selectedCamera.deviceId }, // clave
      width: 1280,
      height: 720
    }
  };

  try {
    const stream = await navigator.mediaDevices.getUserMedia(constraints);
    video.srcObject = stream;

    video.addEventListener("loadeddata", predictWebcam);

    console.log("Usando cámara:", selectedCamera.label);

  } catch (err) {
    console.error("Error al usar cámara:", err);
  }
}

let frameCount = 0;
let lastPose = null;
let lastHands = null;
let lastFace = null;
async function predictWebcam() {
  if (canvasElement.width !== video.videoWidth) {
    canvasElement.width = video.videoWidth;
    canvasElement.height = video.videoHeight;
  }

  if (lastVideoTime !== video.currentTime) {
    lastVideoTime = video.currentTime;
    frameCount++;

    let startTimeMs = performance.now();

    // ACTUALIZAR SOLO CUANDO TOCA
    if (frameCount % 2 === 0) {
      lastPose = await poseLandmarker.detectForVideo(video, startTimeMs);
    }

    if (frameCount % 3 === 0) {
      lastHands = await handLandmarker.detectForVideo(video, startTimeMs);
    }

    if (frameCount % 4 === 0) {
      lastFace = await faceLandmarker.detectForVideo(video, startTimeMs);
    }

    // LIMPIAR SIEMPRE
    canvasCtx.clearRect(0, 0, canvasElement.width, canvasElement.height);

    // DIBUJAR SIEMPRE EL ÚLTIMO RESULTADO
    drawEverything(lastPose, lastHands, lastFace, canvasCtx);
  }

  window.requestAnimationFrame(predictWebcam);
}

const angleDrawer = new AngleDrawer(canvasCtx);  // Angulos
const drawingUtils = new DrawingUtils(canvasCtx); // Dependencia incluida 
function drawEverything(poseResult, handResult, faceResult, ctx) {
  const allowedIndices = [].concat(...Object.values(POSE_PARTS));

  // ================= POSE =================
  if (poseResult && poseResult.landmarks) {
    for (const rawLandmarks of poseResult.landmarks) {

      const adjustedLandmarks = rawLandmarks.map((lm, idx) => {
        let newPos = { ...lm };

        if ([11, 12].includes(idx)) newPos.y -= Y_OFFSET;

        if ([23].includes(idx)) newPos.x -= X_OFFSET;
        if ([24].includes(idx)) newPos.x += X_OFFSET;

        if ([25, 27].includes(idx)) newPos.x -= X_OFFSET_P;
        if ([26, 28].includes(idx)) newPos.x += X_OFFSET_P;

        return newPos;
      });

      const filteredConnections = PoseLandmarker.POSE_CONNECTIONS.filter(conn =>
        allowedIndices.includes(conn.start) && allowedIndices.includes(conn.end)
      );

      drawingUtils.drawConnectors(adjustedLandmarks, filteredConnections, {
        color: "#E0E0E099",
        lineWidth:7
      });

      for (const [partName, indices] of Object.entries(POSE_PARTS)) {
        const partLandmarks = indices.map(index => adjustedLandmarks[index]);
        drawingUtils.drawLandmarks(partLandmarks, {
          color: COLORS[partName],
          lineWidth: 5,
          radius: 10
        });
      }

      // ===== ÁNGULOS =====

      const toPixel = (lm) => ({
        x: lm.x * canvasElement.width,
        y: lm.y * canvasElement.height
      });

      const A1 = toPixel(adjustedLandmarks[11]);
      const B1 = toPixel(adjustedLandmarks[13]);
      const C1 = toPixel(adjustedLandmarks[15]);

      angleDrawer.drawLines(A1, B1, C1);
      angleDrawer.drawAngleArc(A1, B1, C1, 120);
      angleDrawer.drawAngleLabel(A1, B1, C1, 50);

      const A2 = toPixel(adjustedLandmarks[16]);
      const B2 = toPixel(adjustedLandmarks[14]);
      const C2 = toPixel(adjustedLandmarks[12]);

      angleDrawer.drawLines(A2, B2, C2);
      angleDrawer.drawAngleArc(A2, B2, C2, 120);
      angleDrawer.drawAngleLabel(A2, B2, C2, 50);
    }
  }

  // ================= HANDS =================
  if (handResult && handResult.landmarks) {

    const EXCLUDED = [7, 11, 15, 19];

    // conexiones originales filtradas
    let filteredConnections = HandLandmarker.HAND_CONNECTIONS.filter(conn =>
      !EXCLUDED.includes(conn.start) && !EXCLUDED.includes(conn.end)
    );

    // conexiones nuevas para cerrar los huecos
    const extraConnections = [
      { start: 6, end: 8 },
      { start: 10, end: 12 },
      { start: 14, end: 16 },
      { start: 18, end: 20 }
    ];

    filteredConnections = filteredConnections.concat(extraConnections);

    for (const landmarks of handResult.landmarks) {

      // dibujar conexiones
      drawingUtils.drawConnectors(landmarks, filteredConnections, {
        color: COLORS.hands,
        lineWidth: 3
      });

      // filtrar landmarks visibles
      const filteredLandmarks = landmarks.filter((_, i) => !EXCLUDED.includes(i));

      drawingUtils.drawLandmarks(filteredLandmarks, {
        color: "#FFFFFF",
        lineWidth: 1,
        radius: (data) => [4, 8, 12, 16, 20].includes(data.index) ? 4 : 2
      });
    }
  }

  // ================= FACE (SOLO CONTORNO) =================
  if (faceResult && faceResult.faceLandmarks) {
    for (const landmarks of faceResult.faceLandmarks) {
      drawingUtils.drawConnectors(
        landmarks,
        FaceLandmarker.FACE_LANDMARKS_FACE_OVAL,
        {
          color: COLORS.face,
          lineWidth: 4
        }
      );
      const toPixel = (lm) => ({
        x: lm.x * canvasElement.width,
        y: lm.y * canvasElement.height
      });

      // Puntos clave para línea media
      const midPoints = [
        10,   // frente superior
        151,  // frente medio
        9,    // entrecejo
        8,    // puente nariz
        168,  // medio nariz
        6,    // base nariz
        195,  // debajo nariz
        5,    // labio superior
        13,   // centro labios
        14,   // labio inferior
        17,   // mentón superior
        152   // mentón
      ];

      // Dibujar línea conectando todos los puntos
      const points = midPoints.map(idx => toPixel(landmarks[idx]));
      
      ctx.beginPath();
      ctx.moveTo(points[0].x, points[0].y);
      for (let i = 1; i < points.length; i++) {
        ctx.lineTo(points[i].x, points[i].y);
      }
      ctx.strokeStyle = COLORS.face;
      ctx.lineWidth = 4;
      ctx.stroke();
    }
  }
}

webcamButton.addEventListener("click", enableCam);