
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#define PORT 8080
#define MAX_CMDS 10
#define BUFFER_SIZE 4096
// int main() {


//     // Initialize variables for reading input
//     char *input = NULL;
//     size_t len = 0;

//     // Main loop to continuously read and execute commands
//     while (1) {
//         printf("$ ");
//         // Read a line of input
//         ssize_t line = getline(&input, &len, stdin);
//         if (line == -1) {
//             perror("Error reading input");
//             exit(1);
//         }

//         // Remove the newline character from the end of the input
//         input[strcspn(input, "\n")] = 0; 

//         // Check if the user wants to exit
//         if (strcmp(input, "exit") == 0) {
//             break;
//         }

//         // Initialize variables for parsing commands
//         char *commands[MAX_CMDS];
//         int cmd_count = 0;
//         // Split the input into individual commands separated by pipes
//         char *cmd = strtok(input, "|");

//         // Loop through each command
//         while (cmd != NULL && cmd_count < MAX_CMDS) {
//             // Remove leading and trailing spaces from the command
//             while (*cmd == ' ') cmd++;
//             char *end = cmd + strlen(cmd) - 1;
//             while (end > cmd && *end == ' ') end--;
//             *(end + 1) = 0;

//             // Check if the command is not empty
//             if (strlen(cmd) > 0) {
//                 commands[cmd_count++] = cmd;
//             } else {
//                 fprintf(stderr, "Error: Empty command between pipes\n");
//                 cmd_count = 0;
//                 break;
//             }
//             // Move to the next command
//             cmd = strtok(NULL, "|");
//         }

//         // Skip if no valid commands were found
//         if (cmd_count == 0) {
//             continue;
//         }

//         // Execute the command(s)
//         if (cmd_count == 1) {
//             execute_command(commands[0]);
//         } else {
//             execute_piped_commands(commands, cmd_count);
//         }
//     }

//     // Free dynamically allocated memory for input
//     free(input);
//     return 0;
// }



//MOUJA
// int main() {
//     int server_fd, client_fd;
//     struct sockaddr_in address;
//     socklen_t addr_len = sizeof(address);

//     // Create socket
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     // Bind socket to address
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("bind failed");
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 3) < 0) {
//         perror("listen failed");
//         exit(EXIT_FAILURE);
//     }

//     printf("Server listening on port %d\n", PORT);

//     // Accept a client connection
//     if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
//         perror("accept failed");
//         exit(EXIT_FAILURE);
//     }

//     printf("Client connected.\n");

//     char buffer[BUFFER_SIZE] = {0};

//     while (1) {
//         memset(buffer, 0, BUFFER_SIZE);

//         // Read command from socket
//         ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
//         if (valread <= 0) {
//             printf("Client disconnected.\n");
//             break;
//         }

//         buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline if present
//         printf("Received command: %s\n", buffer);

//         if (strcmp(buffer, "exit") == 0) {
//             printf("Exit command received. Closing connection.\n");
//             break;
//         }

//         // Parse command(s)
//         char *commands[MAX_CMDS];
//         int cmd_count = 0;
//         char *cmd = strtok(buffer, "|");

//         while (cmd && cmd_count < MAX_CMDS) {
//             // Trim whitespace
//             while (*cmd == ' ') cmd++;
//             char *end = cmd + strlen(cmd) - 1;
//             while (end > cmd && *end == ' ') end--;
//             *(end + 1) = '\0';

//             if (strlen(cmd) == 0) {
//                 fprintf(stderr, "Empty command found between pipes\n");
//                 cmd_count = 0;
//                 break;
//             }

//             commands[cmd_count++] = cmd;
//             cmd = strtok(NULL, "|");
//         }

//         if (cmd_count == 0) continue;

//         // Redirect output to pipe
//         int pipefd[2];
//         pipe(pipefd);

//         pid_t pid = fork();
//         if (pid == 0) {
//             // Child process
//             dup2(pipefd[1], STDOUT_FILENO);
//             dup2(pipefd[1], STDERR_FILENO);
//             close(pipefd[0]);
//             close(pipefd[1]);

//             if (cmd_count == 1) {
//                 execute_command(commands[0]);
//             } else {
//                 execute_piped_commands(commands, cmd_count);
//             }
//             exit(0);
//         } else {
//             // Parent process
//             close(pipefd[1]);
//             wait(NULL);

//             // Read command output from pipe
//             char output[BUFFER_SIZE] = {0};
//             ssize_t output_len = read(pipefd[0], output, BUFFER_SIZE - 1);
//             close(pipefd[0]);

//             // Send output back to client
//             write(client_fd, output, output_len);
//             printf("[OUTPUT] Sending output to client: %s\n", output);
//         }
//     }

//     // Cleanup
//     close(client_fd);
//     close(server_fd);

//     return 0;
// }

#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Server started, waiting for client connections...\n");

    // Accept a client connection
    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Client connected.\n");

    char buffer[BUFFER_SIZE] = {0};

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        // Read command from socket
        ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (valread <= 0) {
            printf("[INFO] Client disconnected.\n");
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline if present
        printf("[RECEIVED] Received command: \"%s\" from client.\n", buffer);

        if (strcmp(buffer, "exit") == 0) {
            printf("[INFO] Exit command received. Closing connection.\n");
            break;
        }

        // Redirect output to pipe
        int pipefd[2];
        pipe(pipefd);

        pid_t pid = fork();

        if (pid == 0) {
            // Child process
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);
            execute_command(buffer); // Execute the received command
            exit(0);
        } else {
            // Parent process
            close(pipefd[1]);
            int status;
            waitpid(pid, &status, 0); // Wait for child process and get its status

            char output[BUFFER_SIZE] = {0};
            ssize_t output_len = read(pipefd[0], output, BUFFER_SIZE - 1);
            close(pipefd[0]);

            printf("[EXECUTING] Executing command: \"%s\".\n", buffer);
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 127) { // Command not found
                    printf("[ERROR] Command not found: \"%s\".\n", buffer);

                    char error_msg[BUFFER_SIZE];
                    snprintf(error_msg, BUFFER_SIZE, "Command not found: %s", buffer);

                    printf("[OUTPUT] Sending output to client: \"%s\".\n", error_msg);
                    write(client_fd, error_msg, strlen(error_msg));
                } else if (output_len > 0) { // Command executed successfully
                    output[output_len] = '\0';
                    printf("[OUTPUT] Sending output to client:\n%s\n", output);
                    write(client_fd, output, output_len);
                }
            } else {
                printf("[ERROR] Command execution failed unexpectedly.\n");
                write(client_fd,
                    "[ERROR] Command execution failed unexpectedly.\n",
                    strlen("[ERROR] Command execution failed unexpectedly.\n"));
            }

        }
    }

    // Cleanup
    close(client_fd);
    close(server_fd);

    return 0;
}
