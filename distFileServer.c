#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXBUF 256
#define NAMESERVER_PORT 9000
#define NAMESERVER_IP "127.0.0.1"


int read_n(int fd, void *buf, int n) {
    int total = 0;
    while (total < n) {
        int r = read(fd, buf + total, n - total);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}


void split_base_rel(const char *path, char *base, size_t bsz,
                    char *rel, size_t rsz) {

    int len = strlen(path);
    int slash_count = 0;
    int split_pos = -1;

    for (int i = 0; i < len; i++) {
        if (path[i] == '/') {
            slash_count++;
            if (slash_count == 4) {
                split_pos = i;
                break;
            }
        }
    }

    if (split_pos == -1) {
        strncpy(base, path, bsz - 1);
        base[bsz - 1] = '\0';
        rel[0] = '\0';
    } else {
        int blen = split_pos;
        if (blen >= (int)bsz) blen = bsz - 1;
        strncpy(base, path, blen);
        base[blen] = '\0';

        const char *rstart = path + split_pos + 1;
        strncpy(rel, rstart, rsz - 1);
        rel[rsz - 1] = '\0';
    }
}

// CONSULTAR NAMESERVER
int query_name_server(const char *base, char *host, size_t hsz, int *port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(NAMESERVER_PORT);
    inet_pton(AF_INET, NAMESERVER_IP, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    char buf[MAXBUF];
    snprintf(buf, sizeof(buf), "%s\n", base);
    write(sock, buf, strlen(buf));

    memset(buf, 0, sizeof(buf));
    int n = read(sock, buf, sizeof(buf) - 1);
    if (n <= 0) {
        close(sock);
        return -1;
    }
    buf[n] = '\0';
    close(sock);

    if (strcmp(buf, "Unknown") == 0) return -1;

    char *colon = strchr(buf, ':');
    if (!colon) return -1;

    *colon = '\0';
    strncpy(host, buf, hsz - 1);
    host[hsz - 1] = '\0';

    *port = atoi(colon + 1);
    return 0;
}

// CONECTAR A FILESERVER
int connect_file_server(const char *host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

// MAIN DISTFILESERVER
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[MAXBUF];

    int puerto = 8000;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) exit(1);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(puerto);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        exit(1);

    if (listen(server_fd, 5) < 0)
        exit(1);

    printf("distFileServer escuchando en puerto %d\n", puerto);

    while (1) {

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) continue;

        memset(buffer, 0, MAXBUF);
        int n = read(client_fd, buffer, MAXBUF);
        if (n <= 0) {
            close(client_fd);
            continue;
        }
        buffer[n] = '\0';

        printf("Orden cliente: %s\n", buffer);
        // addDir
        if (strncmp(buffer, "addDir ", 7) == 0) {

            char *path = buffer + 7;
            char base[256], rel[256];
            split_base_rel(path, base, sizeof(base), rel, sizeof(rel));

            char host[128];
            int fport;

            if (query_name_server(base, host, sizeof(host), &fport) < 0) {
                int error = -1;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            int fsock = connect_file_server(host, fport);
            if (fsock < 0) {
                int error = -1;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "addDir %s\n", rel);
            write(fsock, cmd, strlen(cmd));

            int result;
            read(fsock, &result, sizeof(int));
            write(client_fd, &result, sizeof(int));

            close(fsock);
        }
        // addFile
        else if (strncmp(buffer, "addFile ", 8) == 0) {

            char *path = buffer + 8;
            char base[256], rel[256];
            split_base_rel(path, base, sizeof(base), rel, sizeof(rel));

            char host[128];
            int fport;

            if (query_name_server(base, host, sizeof(host), &fport) < 0) {
                int error = -2;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            int fsock = connect_file_server(host, fport);
            if (fsock < 0) {
                int error = -2;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            int filesize;
            if (read(client_fd, &filesize, sizeof(int)) <= 0) {
                int error = -2;
                write(client_fd, &error, sizeof(int));
                close(fsock);
                close(client_fd);
                continue;
            }

            char *filebuffer = malloc(filesize);
            if (!filebuffer) {
                int error = -2;
                write(client_fd, &error, sizeof(int));
                close(fsock);
                close(client_fd);
                continue;
            }

            if (read_n(client_fd, filebuffer, filesize) <= 0) {
                int error = -2;
                write(client_fd, &error, sizeof(int));
                free(filebuffer);
                close(fsock);
                close(client_fd);
                continue;
            }

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "addFile %s\n", rel);
            write(fsock, cmd, strlen(cmd));
            write(fsock, &filesize, sizeof(int));
            write(fsock, filebuffer, filesize);

            free(filebuffer);

            int result;
            read(fsock, &result, sizeof(int));
            write(client_fd, &result, sizeof(int));

            close(fsock);
        }

        // getFile
        else if (strncmp(buffer, "getFile ", 8) == 0) {

            char *path = buffer + 8;
            char base[256], rel[256];
            split_base_rel(path, base, sizeof(base), rel, sizeof(rel));

            char host[128];
            int fport;

            if (query_name_server(base, host, sizeof(host), &fport) < 0) {
                int error = -3;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            int fsock = connect_file_server(host, fport);
            if (fsock < 0) {
                int error = -3;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "getFile %s", rel);
            write(fsock, cmd, strlen(cmd));

            int result;
            read(fsock, &result, sizeof(int));
            write(client_fd, &result, sizeof(int));

            if (result == 0) {
                int filesize;
                read(fsock, &filesize, sizeof(int));
                write(client_fd, &filesize, sizeof(int));

                char *filebuffer = malloc(filesize);
                read_n(fsock, filebuffer, filesize);
                write(client_fd, filebuffer, filesize);
                free(filebuffer);
            }

            close(fsock);
        }
        // getFileList
        else if (strncmp(buffer, "getFileList ", 12) == 0) {

            char *path = buffer + 12;
            char base[256], rel[256];
            split_base_rel(path, base, sizeof(base), rel, sizeof(rel));

            char host[128];
            int fport;

            if (query_name_server(base, host, sizeof(host), &fport) < 0) {
                int error = -4;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            int fsock = connect_file_server(host, fport);
            if (fsock < 0) {
                int error = -4;
                write(client_fd, &error, sizeof(int));
                close(client_fd);
                continue;
            }

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "getFileList %s", rel);
            write(fsock, cmd, strlen(cmd));

            int result;
            read(fsock, &result, sizeof(int));
            write(client_fd, &result, sizeof(int));

            if (result == 0) {
                int count;
                read(fsock, &count, sizeof(int));
                write(client_fd, &count, sizeof(int));

                char namebuf[256];
                for (int i = 0; i < count; i++) {
                    memset(namebuf, 0, sizeof(namebuf));
                    int m = read(fsock, namebuf, sizeof(namebuf) - 1);
                    write(client_fd, namebuf, m);
                }
            }

            close(fsock);
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
