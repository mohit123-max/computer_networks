// Simple single-threaded HTTP server in C
// Compile: gcc server.c -o websrv
// Run: ./websrv [html_file] [port]
// Example: ./websrv index.html 8080

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>

#define BACKLOG 10
#define BUF_SZ 4096

// Read whole file into a malloc'ed buffer. Returns size via out_size, or -1 on error.
static long read_file(const char *path, char **out_buf) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long size = ftell(f);
    rewind(f);
    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 1, size, f) != (size_t)size) { free(buf); fclose(f); return -1; }
    buf[size] = '\0';
    fclose(f);
    *out_buf = buf;
    return size;
}

// Send all bytes (loop until done)
static ssize_t send_all(int sock, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        sent += n;
    }
    return sent;
}

int main(int argc, char *argv[]) {
    const char *file_path = NULL;
    int port = 8080;

    if (argc >= 2) file_path = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    char *html_buf = NULL;
    long html_len = 0;

    if (file_path) {
        html_len = read_file(file_path, &html_buf);
        if (html_len < 0) {
            fprintf(stderr, "Warning: cannot read '%s' - using default page\n", file_path);
            html_buf = NULL;
        }
    }

    // default simple HTML if no file given / failed
    const char *default_html =
        "<!doctype html>\n"
        "<html>\n"
        "<head><meta charset=\"utf-8\"><title>Simple C Web Server</title></head>\n"
        "<body>\n"
        "<h1>It works!</h1>\n"
        "<p>This page is served by a minimal C HTTP server.</p>\n"
        "</body>\n"
        "</html>\n";
    if (!html_buf) {
        html_len = strlen(default_html);
        html_buf = malloc(html_len + 1);
        strcpy(html_buf, default_html);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    // allow quick reuse (helps during development)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server listening on 0.0.0.0:%d\n", port);
    printf("Serving %ld bytes as HTML. Press Ctrl+C to stop.\n", html_len);

    while (1) {
        struct sockaddr_in client;
        socklen_t clilen = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr*)&client, &clilen);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
        printf("Accepted connection from %s:%d\n", client_ip, ntohs(client.sin_port));

        // Read request (we won't parse perfectly; just look for GET)
        char buf[BUF_SZ];
        ssize_t r = recv(client_fd, buf, sizeof(buf)-1, 0);
        if (r <= 0) {
            close(client_fd);
            continue;
        }
        buf[r] = '\0';

        // Very simple request check
        if (strncmp(buf, "GET ", 4) == 0) {
            // Build HTTP response
            char header[512];
            int header_len = snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Content-Length: %ld\r\n"
                "Connection: close\r\n"
                "\r\n", html_len);

            // send header then body
            if (send_all(client_fd, header, header_len) < 0) {
                perror("send");
            } else {
                if (send_all(client_fd, html_buf, html_len) < 0) {
                    perror("send body");
                }
            }
        } else {
            // respond 400 for other requests
            const char *bad = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
            send_all(client_fd, bad, strlen(bad));
        }

        close(client_fd);
        printf("Connection closed for %s\n", client_ip);
    }

    free(html_buf);
    close(server_fd);
    return 0;
}


// gcc server.c -o websrv
// ./websrv path/to/yourpage.html 8080

// From another machine on the same network open a browser and go to:
// http://192.168.1.100:8080/
// (Replace 192.168.1.100 and 8080 with your server's IP and chosen port.)

// In the server terminal you can run:
// curl http://127.0.0.1:8080/
// or open http://localhost:8080/ in a browser.