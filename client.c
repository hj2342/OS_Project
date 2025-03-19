#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE];
    char server_response[BUFFER_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server address configuration
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Change if connecting remotely

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server on port %d\n", PORT);

    while (1) {
        printf("$ ");
        if (fgets(command, BUFFER_SIZE, stdin) == NULL) {
            perror("Error reading input");
            break;
        }

        command[strcspn(command, "\n")] = '\0'; // remove newline

        // Send command to server
        if (send(sock, command, strlen(command), 0) < 0) {
            perror("Send failed");
            break;
        }

        if (strcmp(command, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        // Receive output from server
        memset(server_response, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, server_response, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Connection closed by server or error occurred.\n");
            break;
        }

        // Display output from server
        printf("%s\n", server_response);
    }

    close(sock);
    return 0;
}