// client_handler.c
#include "client_handler.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096 // Size of the buffer for reading and writing data
#define MAX_CMDS 10 // Maximum number of commands that can be piped

// Function to handle client connections
void *handle_client(void *arg) {
    // Cast the argument to client_info structure
    client_info *info = (client_info *)arg;
    int client_fd = info->client_fd; // Client file descriptor
    int client_number = info->client_number; // Client number for identification
    struct sockaddr_in client_addr = info->client_addr; // Client address

    // Convert IP and port to readable strings
    char ip[INET_ADDRSTRLEN]; // Buffer to hold the IP address
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN); // Convert IP to string
    int port = ntohs(client_addr.sin_port); // Convert port number to host byte order

    // Free memory allocated in main for client_info structure
    free(info);

    char buffer[BUFFER_SIZE] = {0}; // Buffer to hold incoming data

    while (1) {
        // Clear the buffer for each iteration
        memset(buffer, 0, BUFFER_SIZE);
        // Read data from the client
        ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
        // Check if the client has disconnected
        if (valread <= 0) {
            printf("[INFO] [Client #%d - %s:%d] Client disconnected.\n", client_number, ip, port);
            break;
        }

        // Remove newline characters from the buffer
        buffer[strcspn(buffer, "\r\n")] = '\0';

        // Check if the buffer contains only whitespace or control characters
        int all_whitespace = 1, has_control = 0;
        for (size_t i = 0; i < strlen(buffer); i++) {
            if (buffer[i] < 32 || buffer[i] == 127) has_control = 1; // Check for control characters
            if (buffer[i] != ' ') all_whitespace = 0; // Check for non-whitespace characters
        }

        // Handle invalid or unsupported input
        if (strlen(buffer) == 0 || all_whitespace || has_control) {
            const char *err_msg = "Error: Invalid or unsupported input.\n";
            write(client_fd, err_msg, strlen(err_msg));
            printf("[WARNING] [Client #%d - %s:%d] Ignored invalid input: \"%s\"\n", client_number, ip, port, buffer);
            continue;
        }

        // Log received command
        printf("[RECEIVED]  [Client #%d - %s:%d] Received command: \"%s\"\n", client_number, ip, port, buffer);

        // Handle exit command
        if (strcmp(buffer, "exit") == 0) {
            printf("[INFO] [Client #%d - %s:%d] Exit command received. Closing connection.\n", client_number, ip, port);
            break;
        }

        // Parse commands separated by pipes
        char *commands[MAX_CMDS];
        int cmd_count = 0;
        char original_cmd[BUFFER_SIZE];
        strncpy(original_cmd, buffer, BUFFER_SIZE - 1);
        original_cmd[BUFFER_SIZE - 1] = '\0';
        char *cmd = strtok(buffer, "|");
        int error_detected = 0;

        while (cmd && cmd_count < MAX_CMDS) {
            // Remove leading and trailing spaces from the command
            while (*cmd == ' ') cmd++;
            char *end = cmd + strlen(cmd) - 1;
            while (end > cmd && *end == ' ') end--;
            *(end + 1) = '\0';

            // Check for empty commands
            if (strlen(cmd) == 0) {
                error_detected = 1;
                break;
            }

            // Store the command
            commands[cmd_count++] = cmd;
            cmd = strtok(NULL, "|");
        }

        // Handle errors in command parsing
        if (error_detected || cmd_count == 0 || original_cmd[0] == '|') {
            const char *error_message;
            if (original_cmd[0] == '|') {
                error_message = "Error: Missing command before pipe.\n";
            } else if (cmd_count > 0 && strlen(commands[cmd_count - 1]) == 0) {
                error_message = "Error: Missing command after pipe.\n";
            } else {
                error_message = "Error: Empty command between pipes.\n";
            }

            fprintf(stderr, "[ERROR] [Client #%d - %s:%d] %s", client_number, ip, port, error_message);
            write(client_fd, error_message, strlen(error_message));
            printf("[OUTPUT]    [Client #%d - %s:%d] Sending error message to client: \"%s\"\n", client_number, ip, port, error_message);
            continue;
        }

        // Create a pipe for communication between parent and child processes
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            perror("[ERROR] Pipe creation failed");
            continue;
        }

        // Fork a child process to execute the command
        pid_t pid = fork();
        if (pid == 0) {
            // Redirect stdout and stderr to the pipe
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            // Log command execution
            printf("[EXECUTING] [Client #%d - %s:%d] Executing command: \"%s\"\n", client_number, ip, port, original_cmd);
            // Execute the command
            execlp("sh", "sh", "-c", original_cmd, (char *)NULL);
            perror("execlp");
            exit(1);
        } else {
            // Close the write end of the pipe in the parent process
            close(pipefd[1]);
            char output[BUFFER_SIZE * 4] = {0}; // Buffer to hold the output
            char temp[BUFFER_SIZE] = {0}; // Temporary buffer for reading
            ssize_t bytes_read;
            int total_len = 0;

            // Read output from the child process
            while ((bytes_read = read(pipefd[0], temp, sizeof(temp) - 1)) > 0) {
                memcpy(output + total_len, temp, bytes_read);
                total_len += bytes_read;
                if (total_len >= sizeof(output)) break;
            }

            // Close the read end of the pipe
            close(pipefd[0]);
            int status;
            waitpid(pid, &status, 0);

            // Handle command execution status
            if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
                write(client_fd, output, strlen(output));
            } else {
                if (total_len == 0) {
                    const char *msg = "[INFO] Command executed with no output.\n";
                    write(client_fd, msg, strlen(msg));
                    printf("[OUTPUT]    [Client #%d - %s:%d] Command executed with no output.\n", client_number, ip, port);
                } else {
                    write(client_fd, output, total_len);
                    printf("[OUTPUT]    [Client #%d - %s:%d] Sending output to client:\n%.*s\n", client_number, ip, port, total_len, output);
                }
            }
        }
    }

    // Close the client file descriptor
    close(client_fd);
    return NULL;
}
