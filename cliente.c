#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

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



int write_n(int fd, void *buf, int n) {
    int total = 0;
    while (total < n) {
        int w = write(fd, buf + total, n - total);
        if (w <= 0) return w;
        total += w;
    }
    return total;
}

int main() {
    char comando[MAXBUF];

    while (1) {
        printf("cliente> ");
        fflush(stdout);

        if (!fgets(comando, MAXBUF, stdin))
            break;

        comando[strcspn(comando, "\n")] = '\0';

        if (strcmp(comando, "exit") == 0)
            break;

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            continue;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(8000);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("connect");
            close(sock);
            continue;
        }

        write_n(sock, comando, strlen(comando));

        // addFile
        if (strncmp(comando, "addFile ", 8) == 0) {
            int size;
            printf("Tamaño del archivo: ");
            scanf("%d", &size);
            getchar(); // limpiar \n

            char *contenido = malloc(size);
            if (!contenido) {
                printf("no hay memoria\n");
                close(sock);
                continue;
            }

            printf("Contenido (%d bytes exactos): ", size);
            fflush(stdout);

            int leidos = read_n(STDIN_FILENO, contenido, size);
            if (leidos != size) {
                printf("\nError");
                free(contenido);
                close(sock);
                continue;
            }

            write_n(sock, &size, sizeof(int));
            write_n(sock, contenido, size);

            free(contenido);
        }

        // Recibir respuesta
        int result;
        if (read_n(sock, &result, sizeof(int)) <= 0) {
            printf("Error leyendo respuesta\n");
            close(sock);
            continue;
        }

        printf("Resultado: %d\n", result);

        // getFile
        if (strncmp(comando, "getFile ", 8) == 0 && result == 0) {
            int size;
            read_n(sock, &size, sizeof(int));

            char *contenido = malloc(size);
            read_n(sock, contenido, size);

            printf("Contenido del archivo (%d bytes):\n", size);
            fwrite(contenido, 1, size, stdout);
            printf("\n");

            free(contenido);
        }

        // getFileList
        if (strncmp(comando, "getFileList ", 12) == 0 && result == 0) {
            int count;
            read_n(sock, &count, sizeof(int));

            printf("Archivos (%d):\n", count);

            char nombre[256];
            for (int i = 0; i < count; i++) {
                memset(nombre, 0, sizeof(nombre));
                int m = read(sock, nombre, sizeof(nombre) - 1);
                printf(" - %s", nombre);
            }
            printf("\n");
        }

        close(sock);
    }

    return 0;
}
