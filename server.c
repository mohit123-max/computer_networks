// multi_client_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_sock)
{
    char buffer[BUFFER_SIZE];
    int n;

    while ((n = read(client_sock, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[n] = '\0';
        printf("Received from client: %s\n", buffer);

        // Echo back to client
        write(client_sock, buffer, n);
    }

    if (n == 0)
    {
        printf("Client disconnected.\n");
    }
    else if (n < 0)
    {
        perror("Read error");
    }

    close(client_sock);
    exit(0); // End child process
}

int main()
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    pid_t pid;

    // Ignore SIGCHLD to prevent zombie processes
    signal(SIGCHLD, SIG_IGN);

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prepare sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_sock, 5) < 0)
    {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1)
    {
        client_len = sizeof(client_addr);
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Connection accepted from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid = fork();

        if (pid < 0)
        {
            perror("Fork failed");
            close(client_sock);
        }
        else if (pid == 0)
        {
            // Child process
            close(server_sock); // Close listening socket in child
            handle_client(client_sock);
        }
        else
        {
            // Parent process
            close(client_sock); // Close client socket in parentkyu 
        }
    }

    close(server_sock);
    return 0;
}