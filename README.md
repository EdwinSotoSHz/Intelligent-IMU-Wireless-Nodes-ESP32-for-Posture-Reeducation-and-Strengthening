# Intelligent-IMU-Wireless-Nodes-ESP32-for-Posture-Reeducation-and-Strengthening

docker volume ls
docker volume 
docker volume rm <name>

docker-compose down
docker-compose up -d --build



entar al contenedor:
docker exec -it nodered bash
cd /data
ls



Exportar volumenes:
docker run --rm `
-v releases_influxdb-data:/influxdb-data `
-v releases_influxdb-config:/influxdb-config `
-v releases_grafana-data:/grafana-data `
-v releases_node-red-data:/node-red-data `
-v ${PWD}:/backup `
alpine `
tar czf /backup/backup_full.tar.gz `
/influxdb-data /influxdb-config /grafana-data /node-red-data

Borrar volumenes viejos:
docker-compose down

docker volume rm releases_influxdb-data
docker volume rm releases_influxdb-config
docker volume rm releases_grafana-data
docker volume rm releases_node-red-data

docker volume ls

Restaurar:
docker run --rm `
-v releases_influxdb-data:/influxdb-data `
-v releases_influxdb-config:/influxdb-config `
-v releases_grafana-data:/grafana-data `
-v releases_node-red-data:/node-red-data `
-v ${PWD}:/backup alpine tar xzf /backup/backup_full.tar.gz -C /

Crear compose:
docker-compose up -d