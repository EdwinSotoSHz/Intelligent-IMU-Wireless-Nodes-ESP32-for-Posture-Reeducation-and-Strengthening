import {
  PoseLandmarker,
  HandLandmarker,
  FaceLandmarker,
  FilesetResolver,
  DrawingUtils
} from "https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@0.10.0";

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
    minTrackingConfidence: 0.8
  });

  handLandmarker = await HandLandmarker.createFromOptions(vision, {
    baseOptions: {
      modelAssetPath: `https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task`,
      delegate: "GPU"
    },
    runningMode: "VIDEO",
    numHands: 2,
    minHandDetectionConfidence: 0.8,
    minHandPresenceConfidence: 0.8,
    minTrackingConfidence: 0.8
  });

  // solo contorno
  faceLandmarker = await FaceLandmarker.createFromOptions(vision, {
    baseOptions: {
      modelAssetPath: `https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task`,
      delegate: "GPU"
    },
    runningMode: "VIDEO",
    minFaceDetectionConfidence: 0.4,
    minTrackingConfidence: 0.3,
    numFaces: 1
  });
};

setupModels();

let currentCameraIndex = 2; // CAM
async function enableCam() {
  if (!poseLandmarker || !handLandmarker || !faceLandmarker) return;

  webcamButton.style.display = "none";

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

  // 🔴 IMPORTANTE: detener cámara anterior
  if (video.srcObject) {
    video.srcObject.getTracks().forEach(track => track.stop());
  }

  const constraints = {
    video: {
      deviceId: { exact: selectedCamera.deviceId }, // 🔥 clave
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

async function predictWebcam() {
  if (canvasElement.width !== video.videoWidth) {
    canvasElement.width = video.videoWidth;
    canvasElement.height = video.videoHeight;
  }

  let startTimeMs = performance.now();

  if (lastVideoTime !== video.currentTime) {
    lastVideoTime = video.currentTime;

    const [poseResult, handResult, faceResult] = await Promise.all([
      poseLandmarker.detectForVideo(video, startTimeMs),
      handLandmarker.detectForVideo(video, startTimeMs),
      faceLandmarker.detectForVideo(video, startTimeMs)
    ]);

    canvasCtx.clearRect(0, 0, canvasElement.width, canvasElement.height);
    drawEverything(poseResult, handResult, faceResult, canvasCtx);
  }

  window.requestAnimationFrame(predictWebcam);
}

function drawEverything(poseResult, handResult, faceResult, ctx) {
  const drawingUtils = new DrawingUtils(ctx);
  const allowedIndices = [].concat(...Object.values(POSE_PARTS));

  // ================= POSE =================
  if (poseResult.landmarks) {
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

  // ================= HANDS =================
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

  // ================= FACE (SOLO CONTORNO) =================
  if (faceResult && faceResult.faceLandmarks) {
    for (const landmarks of faceResult.faceLandmarks) {
      drawingUtils.drawConnectors(
        landmarks,
        FaceLandmarker.FACE_LANDMARKS_FACE_OVAL,
        {
          color: COLORS.face,
          lineWidth: 2
        }
      );
    }
  }
}

webcamButton.addEventListener("click", enableCam);