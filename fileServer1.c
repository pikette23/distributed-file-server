#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define MAXBUF 256


int read_n(int fd, void *buf, int n) {
    int total = 0;
    while (total < n) {
        int r = read(fd, buf + total, n - total);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Uso: %s <ruta_base> <puerto>\n", argv[0]);
        exit(1);
    }

    char *ruta_base = argv[1];
    int puerto = atoi(argv[2]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(puerto);

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("bind");
        exit(1);
    }

    if(listen(server_fd,5) < 0){
        perror("listen");
        exit(1);
    }

    printf("fileServer escuchando en puerto %d para ruta %s\n", puerto, ruta_base);

    while (1){

        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        char buffer[MAXBUF];
        memset(buffer,0, MAXBUF);
        int n = read(client_fd, buffer, MAXBUF - 1);
        if (n <= 0) {
            close(client_fd);
            continue;
        }
        buffer[n] = '\0';

        printf("Orden recibida: %s\n", buffer);

        // addDir
        if(strncmp(buffer, "addDir ", 7) == 0){
            char *subdir = buffer + 7;

            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", ruta_base, subdir);

            int result = mkdir(fullpath, 0777);

            if(result == 0){
                int ok = 0;
                write(client_fd, &ok, sizeof(int));
            } else {
                int error = -1;
                write(client_fd, &error, sizeof(int));
            }
        }
        // addFile
        else if(strncmp(buffer, "addFile ", 8) == 0){
            char *filename = buffer + 8;

            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", ruta_base, filename);

            int filesize;
            if(read_n(client_fd, &filesize, sizeof(int)) <= 0){
                int error = -2;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            char *filebuffer = malloc(filesize);
            if (!filebuffer) {
                int error = -2;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            if (read_n(client_fd, filebuffer, filesize) <= 0) {
                int error = -2;
                write(client_fd, &error, sizeof(int));
                free(filebuffer);
                close(client_fd);
                continue;
            }

            FILE *f = fopen(fullpath, "wb");
            if (!f) {
                int error = -2;
                write(client_fd, &error, sizeof(int));
                free(filebuffer);
                close(client_fd);
                continue;
            }

            fwrite(filebuffer, 1, filesize, f);
            fclose(f);
            free(filebuffer);

            int ok = 0;
            write(client_fd, &ok, sizeof(int));
        }

        
        // getFile
        
        else if (strncmp(buffer, "getFile ", 8) == 0) {

            char *filename = buffer + 8;
            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", ruta_base, filename);

            FILE *f = fopen(fullpath, "rb");
            if (!f) {
                int error = -3;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            int ok = 0;
            write(client_fd, &ok, sizeof(int));

            fseek(f, 0, SEEK_END);
            int filesize = ftell(f);
            fseek(f, 0, SEEK_SET);

            write(client_fd, &filesize, sizeof(int));

            char *bufferfile = malloc(filesize);
            fread(bufferfile, 1, filesize, f);
            write(client_fd, bufferfile, filesize);

            free(bufferfile);
            fclose(f);
        }

      
        // getFileList
        
        else if (strncmp(buffer, "getFileList ", 12) == 0) {

            char *dirname = buffer + 12;

            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", ruta_base, dirname);

            DIR *dir = opendir(fullpath);
            if (!dir) {
                int error = -4;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            int ok = 0;
            write(client_fd, &ok, sizeof(int));

            struct dirent *entry;
            int count = 0;

            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") != 0 &&
                    strcmp(entry->d_name, "..") != 0)
                    count++;
            }

            write(client_fd, &count, sizeof(int));

            rewinddir(dir);

            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") != 0 &&
                    strcmp(entry->d_name, "..") != 0) {
                    write(client_fd, entry->d_name, strlen(entry->d_name));
                    write(client_fd, "\n", 1);
                }
            }

            closedir(dir);
        }

        close(client_fd);
    }
}
