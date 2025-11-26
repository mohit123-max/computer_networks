//gcc server.c -o server -lpthread

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 100

int clients[MAX_CLIENTS];
char usernames[MAX_CLIENTS][50];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void broadcast(char *msg, int sender_fd) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != -1 && clients[i] != sender_fd) {
            send(clients[i], msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[1024];
    char name[50];

    // Read username first
    recv(client_fd, name, sizeof(name), 0);
    name[strcspn(name, "\n")] = 0;

    // Save username
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_fd) {
            strcpy(usernames[i], name);
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    // Announce join
    char join_msg[200];
    sprintf(join_msg, "%s joined the chat\n", name);
    broadcast(join_msg, client_fd);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_fd, buffer, sizeof(buffer), 0);

        if (bytes <= 0) {
            // Announce leaving
            char leave_msg[200];
            sprintf(leave_msg, "%s left the chat\n", name);
            broadcast(leave_msg, client_fd);

            close(client_fd);

            pthread_mutex_lock(&lock);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == client_fd) {
                    clients[i] = -1;
                    break;
                }
            }
            pthread_mutex_unlock(&lock);
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        // Broadcast message with username
        char msg[1200];
        sprintf(msg, "%s: %s\n", name, buffer);
        broadcast(msg, client_fd);
    }

    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);

    // Initialize client list
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i] = -1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket error");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&server, sizeof(server));
    listen(server_fd, 10);

    printf("Server started... Waiting for clients...\n");

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        pthread_mutex_lock(&lock);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == -1) {
                clients[i] = client_fd;
                break;
            }
        }
        pthread_mutex_unlock(&lock);

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void *)&client_fd);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}
