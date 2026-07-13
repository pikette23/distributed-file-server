# Distributed File Server (C)

Este proyecto implementa una **simulación de un sistema distribuido de archivos** compuesto por:

- **nameServer**: resuelve qué servidor gestiona cada ruta.
- **fileServer**: gestiona directorios y archivos de un dominio concreto.
- **distFileServer**: actúa como intermediario entre el cliente y los servidores.
- **cliente**: interfaz interactiva para enviar órdenes al sistema distribuido.

El objetivo es permitir operaciones básicas sobre archivos y directorios distribuidos entre varias rutas, utilizando comunicación mediante sockets TCP.

---

## 🚀 Ejecución

### 1. Lanzar el nameServer:
./nameServer

### 2. Lanzar los fileServers:
./fileServer1 /home/robert/ruta1 10000

./fileServer1 /home/robert/ruta2 10001

./fileServer1 /home/robert/ruta3 10002

### 3. Lanzar el servidor distribuido:
./distFileServer

### 4. Ejecutar el cliente:
./cliente

### Órdenes disponibles
addDir <RutaCompleta>
addFile <ArchivoLocal> <RutaCompleta>
getFile <RutaCompleta>
getFileList <RutaCompleta>

## 📸 Ejemplos de funcionamiento
### Crear un directorio:
Cliente:
cliente> addDir /home/robert/ruta1/d1
Resultado: 0
FileServer responde:
Orden recibida: addDir d1

### Subir y recuperar un archivo
cliente> addFile /home/robert/ruta1/d1/a.txt
Resultado: 0
Descarga:
cliente> getFile /home/robert/ruta1/d1/a.txt
Resultado: 0
Contenido del archivo (5 bytes): hola!


