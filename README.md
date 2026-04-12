Aquí tienes una versión limpia y documentada para tu README, manteniendo un estilo técnico y directo.

---

# 🧠 Intelligent IMU Wireless Nodes - Docker Setup

## 📋 Descripción

Este proyecto utiliza contenedores Docker para ejecutar:

* Node-RED (procesamiento de datos y MQTT)
* WebServices (backend)
* InfluxDB (base de datos time-series)
* Grafana (visualización)

Incluye persistencia mediante volúmenes y soporte para backup/restore completo.

---

## 🚀 Comandos Básicos

### 🔹 Volúmenes

```bash
docker volume ls                 # Listar volúmenes
docker volume inspect <name>    # Ver detalles de un volumen
docker volume rm <name>         # Eliminar volumen
```

---

### 🔹 Docker Compose

```bash
docker-compose up -d --build    # Construir y levantar servicios
docker-compose down             # Detener y eliminar contenedores
```

---

## 🐳 Acceso a Contenedores

### Entrar a Node-RED

```bash
docker exec -it nodered bash
```

### Navegar en datos persistentes

```bash
cd /data
ls
```

---

## 💾 Backup de Volúmenes

Exporta todos los datos (InfluxDB, Grafana, Node-RED):

```powershell
docker run --rm `
-v releases_influxdb-data:/influxdb-data `
-v releases_influxdb-config:/influxdb-config `
-v releases_grafana-data:/grafana-data `
-v releases_node-red-data:/node-red-data `
-v ${PWD}:/backup `
alpine `
tar czf /backup/backup_full.tar.gz `
/influxdb-data /influxdb-config /grafana-data /node-red-data
```

Archivo generado:

```text
backup_full.tar.gz
```

---

## 🧹 Eliminar Volúmenes (Reset completo)

```bash
docker-compose down

docker volume rm releases_influxdb-data
docker volume rm releases_influxdb-config
docker volume rm releases_grafana-data
docker volume rm releases_node-red-data

docker volume ls
```

---

## ♻️ Restaurar Backup

```powershell
docker run --rm `
-v releases_influxdb-data:/influxdb-data `
-v releases_influxdb-config:/influxdb-config `
-v releases_grafana-data:/grafana-data `
-v releases_node-red-data:/node-red-data `
-v ${PWD}:/backup `
alpine `
tar xzf /backup/backup_full.tar.gz -C /
```

---

## ▶️ Levantar el Sistema

```bash
docker-compose up -d
```

---

## 🌐 Accesos

| Servicio    | URL                                              |
| ----------- | ------------------------------------------------ |
| Node-RED    | [http://localhost:1880/](http://localhost:1880/) |
| WebServices | [http://localhost:3000/](http://localhost:3000/) |
| InfluxDB    | [http://localhost:8086/](http://localhost:8086/) |
| Grafana     | [http://localhost:3001/](http://localhost:3001/) |

---

## 🧠 Notas Importantes

* Los volúmenes contienen:

  * InfluxDB: datos + configuración
  * Grafana: dashboards
  * Node-RED: flows y credenciales

* El backup es binario:

  * rápido
  * completo
  * no requiere reconfiguración

* Para restaurar correctamente:

  * eliminar volúmenes antes
  * luego ejecutar restore

---

## 📁 Requisitos para replicar el entorno

* `docker-compose.yml`
* carpetas de build (`nodered`, `webservices`)
* archivo de backup (`backup_full.tar.gz`)

---

## ⚠️ Consideraciones

* Asegurar mismas versiones de contenedores
* No modificar volúmenes manualmente
* Certificados TLS deben estar en rutas persistentes 

---