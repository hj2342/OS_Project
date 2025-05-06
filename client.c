#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char command[BUFFER_SIZE];


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

        // Set a timeout for receiving response
        struct timeval tv;
        tv.tv_sec = 1;  // 1 second timeout
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        // Receive output from server
        // Check if command has redirection
        int has_redirection = (strchr(command, '>') != NULL);

        // Always use blocking I/O
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);

        // Set timeout based on command type
        int is_program = strncmp(command, "./", 2) == 0;
        struct timeval cmd_timeout = {
            .tv_sec = is_program ? 60 : 1,  // 60s for programs (to handle preemption), 1s for shell commands
            .tv_usec = 0
        };
        struct timeval original_timeout = cmd_timeout;  // Keep original for resets
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &cmd_timeout, sizeof(cmd_timeout));

        // Read response from server
        char response[BUFFER_SIZE];
        ssize_t total_received = 0;
        ssize_t bytes_received = 0;
        int task_completed = 0;

        while (!task_completed) {
            bytes_received = recv(sock, response, BUFFER_SIZE - 1, 0);
            
            if (bytes_received > 0) {
                response[bytes_received] = '\0';
                
                // Check for status messages
                if (strncmp(response, "[STATUS] ", 9) == 0) {
                    if (strstr(response, "COMPLETED")) {
                        task_completed = 1;
                        continue;
                    } else if (strstr(response, "PREEMPTED")) {
                        // Task was preempted, reset timeout and keep waiting
                        cmd_timeout = original_timeout;
                        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &cmd_timeout, sizeof(cmd_timeout));
                        continue;
                    }
                }

                // Write regular output
                write(STDOUT_FILENO, response, bytes_received);
                total_received += bytes_received;

                // For redirected commands, we're done after output
                if (has_redirection) {
                    break;
                }
            } else if (bytes_received == 0) {
                if (total_received == 0) {
                    printf("No output from command.\n");
                }
                break;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (is_program) {
                    if (!task_completed) {
                        // Reset timeout and keep waiting
                        cmd_timeout = original_timeout;
                        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &cmd_timeout, sizeof(cmd_timeout));
                        continue;
                    }
                } else if (total_received > 0 || has_redirection) {
                    // For non-program commands, exit if we've received something
                    break;
                }
                continue;
            } else if (errno != EINTR) {
                perror("recv failed");
                break;
            }
        }

        // Reset the timeout
        tv.tv_sec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        // Only break the main loop if there was an actual error
        if (bytes_received < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            printf("Connection error: %s\n", strerror(errno));
            break;
        }
    }

    close(sock);
    return 0;
}