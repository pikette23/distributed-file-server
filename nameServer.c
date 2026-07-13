#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000 //Puerto donde escuchará el nameServer
#define MAXBUF 256

/*APARTADO 1*/
struct RutaServidor{
  char *ruta;
  char *direccion; 
};

//Tabla estática
struct RutaServidor tabla[] = {
  {"/home/robert/ruta1", "localhost:10000"},
  {"/home/robert/ruta2", "localhost:10001"},
  {"/home/robert/ruta3", "localhost:10002"}
};
int tabla_size = 3;

//Buscamos la ruta en la tabla
char *getFileServer(char *path){
  for(int i= 0; i < tabla_size; i++){
    if(strcmp(path, tabla[i].ruta) == 0){
      return tabla[i].direccion;
    }
  }
  return "Unknown";
}
int main(){
  //server_fd: socket pasivo, client_fd: socket activo
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_len = sizeof(client_addr);
  char buffer[MAXBUF];
  
  //Creamos socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd < 0){
    perror("socket");
    exit(1);
  }
  
  //Configurar dirección del servidor
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;//Aceptar conexión en cualquier ip local
  server_addr.sin_port = htons(PORT);
  
  //Asignamos una dirección a nivel de capa de transporte: IP + puerto
  if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))){
    perror("bind");
    exit(1);
  }
  
  //Escuchar conexiones
  if(listen(server_fd, 5) < 0 ){
    perror("listen");
    exit(1);
  }
  printf("nameServer escuchando en puerto %d",PORT);
  
  while (1) {
        // Aceptar cliente
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // Recibir ruta
        memset(buffer, 0, MAXBUF);
        
        ssize_t len = read(client_fd, buffer, MAXBUF);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';  // Eliminar salto de línea
        }

        printf("Consulta recibida: %s\n", buffer);

        // Buscar servidor
        char *respuesta = getFileServer(buffer);

        // Enviar respuesta
        write(client_fd, respuesta, strlen(respuesta));

        close(client_fd);
    }

    close(server_fd);
    return 0;
  
}


