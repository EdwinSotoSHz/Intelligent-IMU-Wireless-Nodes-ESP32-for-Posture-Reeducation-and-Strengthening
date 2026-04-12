const video = document.getElementById("webcam");

// 1. Pedir permiso primero (esto desbloquea los labels)
async function initCameraDebug() {
  try {
    const tempStream = await navigator.mediaDevices.getUserMedia({ video: true });
    
    // Detenemos inmediatamente (solo era para permisos)
    tempStream.getTracks().forEach(track => track.stop());

    listCameras();
  } catch (err) {
    console.error("Error al pedir permisos:", err);
  }
}

// 2. Listar cámaras
async function listCameras() {
  const devices = await navigator.mediaDevices.enumerateDevices();
  const videoDevices = devices.filter(d => d.kind === "videoinput");

  console.log("=== CÁMARAS DISPONIBLES ===");

  videoDevices.forEach((device, index) => {
    console.log(`
📷 Cámara ${index}
Nombre: ${device.label}
deviceId: ${device.deviceId}
-------------------------
    `);
  });

  console.table(videoDevices);
}

// 3. Función para probar cambiar de cámara manualmente
async function useCamera(index) {
  const devices = await navigator.mediaDevices.enumerateDevices();
  const videoDevices = devices.filter(d => d.kind === "videoinput");

  const selected = videoDevices[index];

  if (!selected) {
    console.error("Índice inválido");
    return;
  }

  // 🔴 detener stream anterior
  if (video.srcObject) {
    video.srcObject.getTracks().forEach(track => track.stop());
  }

  const stream = await navigator.mediaDevices.getUserMedia({
    video: {
      deviceId: { exact: selected.deviceId }
    }
  });

  video.srcObject = stream;

  console.log("Usando cámara:", selected.label);
}

// Inicializar
initCameraDebug();