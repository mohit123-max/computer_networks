#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Setup address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started... waiting for connection...\n");

    // Accept
    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    printf("Client connected!\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));

        // Receive message
        int valread = read(new_socket, buffer, 1024);
        if (valread <= 0) {
            printf("Client disconnected.\n");
            close(new_socket);
            close(server_fd);
            break;
        }

        printf("Client: %s", buffer);

        // Server replies
        char msg[1024];
        printf("Server: ");
        fgets(msg, sizeof(msg), stdin);

        // Exit condition
        if (strncmp(msg, "exit", 4) == 0) {
            printf("Server shutting down...\n");
            send(new_socket, msg, strlen(msg), 0);
            close(new_socket);
            close(server_fd);
            break;
        }

        send(new_socket, msg, strlen(msg), 0);
    }

    return 0;
}
