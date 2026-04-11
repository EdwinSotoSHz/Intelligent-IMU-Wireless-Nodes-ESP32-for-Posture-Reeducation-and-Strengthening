const express = require("express");
const path = require("path");
const http = require("http");
const WebSocket = require("ws");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Raiz
app.use(express.static(path.join(__dirname, "..")));

// Servir archivos estáticos
app.use("/MediaPipe", express.static(path.join(__dirname, "../MediaPipe")));
app.use("/Threejs", express.static(path.join(__dirname, "../Threejs")));

// WebSocket
wss.on("connection", (ws) => {
    console.log("Cliente conectado");

    ws.on("message", (msg) => {
        console.log("Mensaje:", msg.toString());

        // eco (ejemplo)
        ws.send(`Recibido: ${msg}`);
    });

    ws.on("close", () => {
        console.log("Cliente desconectado");
    });
});

// Puerto
const PORT = 3000;
server.listen(PORT, () => {
    console.log(`Servidor en http://localhost:${PORT}`);
});