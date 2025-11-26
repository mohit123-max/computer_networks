#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080

//127.0.0.1 - loopback ip

int sock;

void *receive_messages(void *arg) {
    char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            printf("Disconnected from server\n");
            exit(0);
        }
        printf("%s", buffer);
    }
}

int main() {
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    printf("Enter server IP: ");
    char ip[50];
    scanf("%s", ip);

    inet_pton(AF_INET, ip, &server.sin_addr);

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    // Send username
    printf("Enter your username: ");
    char username[50];
    scanf("%s", username);
    send(sock, username, strlen(username), 0);

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_detach(recv_thread);

    // Send messages
    char message[1024];
    getchar(); // remove newline from buffer
    while (1) {
        fgets(message, sizeof(message), stdin);
        send(sock, message, strlen(message), 0);
    }
}
