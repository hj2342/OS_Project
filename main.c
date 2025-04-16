
// #include "executor.h" // Include the header file for the executor functions
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <netinet/in.h>
// #include <sys/wait.h>

// #define PORT 8080
// #define MAX_CMDS 10
// #define BUFFER_SIZE 4096

// void handle_client(int client_fd) {
//     char buffer[BUFFER_SIZE] = {0};

//     while (1) {
//         memset(buffer, 0, BUFFER_SIZE);

//         ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
//         if (valread <= 0) {
//             printf("[INFO] Client disconnected.\n");
//             break;
//         }

//         // Strip newline and carriage return
//         buffer[strcspn(buffer, "\r\n")] = '\0';

//         // Ignore empty input or input with control characters
//         int all_whitespace = 1, has_control = 0;
//         for (int i = 0; i < strlen(buffer); i++) {
//             if (buffer[i] < 32 || buffer[i] == 127) {
//                 has_control = 1;
//             }
//             if (buffer[i] != ' ') {
//                 all_whitespace = 0;
//             }
//         }

//         if (strlen(buffer) == 0 || all_whitespace || has_control) {
//             char *err_msg = "Error: Invalid or unsupported input.\n";
//             write(client_fd, err_msg, strlen(err_msg));
//             printf("[WARNING] Ignored invalid input: \"%s\"\n", buffer);
//             continue;
//         }
//         printf("[RECEIVED] Received command: \"%s\" from client.\n", buffer);

//         if (strcmp(buffer, "exit") == 0) {
//             printf("[INFO] Exit command received. Closing client connection.\n");
//             break;
//         }

//         char *commands[MAX_CMDS];
//         int cmd_count = 0;
//         char original_cmd[BUFFER_SIZE];
//         strncpy(original_cmd, buffer, BUFFER_SIZE - 1);
//         original_cmd[BUFFER_SIZE - 1] = '\0';
//         char *cmd = strtok(buffer, "|");
//         int error_detected = 0;

//         while (cmd && cmd_count < MAX_CMDS) {
//             while (*cmd == ' ') cmd++;
//             char *end = cmd + strlen(cmd) - 1;
//             while (end > cmd && *end == ' ') end--;
//             *(end + 1) = '\0';

//             if (strlen(cmd) == 0) {
//                 error_detected = 1;
//                 break;
//             }

//             commands[cmd_count++] = cmd;
//             cmd = strtok(NULL, "|");
//         }

//         if (error_detected || cmd_count == 0 || buffer[0] == '|') {
//             char *error_message;
//             if (buffer[0] == '|') {
//                 error_message = "Error: Missing command before pipe.\n";
//             } else if (cmd_count > 0 && strlen(commands[cmd_count - 1]) == 0) {
//                 error_message = "Error: Missing command after pipe.\n";
//             } else {
//                 error_message = "Error: Empty command between pipes.\n";
//             }

//             fprintf(stderr, "[ERROR] %s", error_message);
//             write(client_fd, error_message, strlen(error_message));
//             printf("[OUTPUT] Sending error message to client: \"%s\"\n", error_message);
//             continue;
//         }

//         int pipefd[2];
//         if (pipe(pipefd) < 0) {
//             perror("[ERROR] Pipe creation failed");
//             continue;
//         }

//         pid_t pid = fork();

//         if (pid == 0) {
//             dup2(pipefd[1], STDOUT_FILENO);
//             dup2(pipefd[1], STDERR_FILENO);
//             close(pipefd[0]);
//             close(pipefd[1]);

//             // int exit_status = (cmd_count == 1)
//             //                   ? execute_command(commands[0])
//             //                   : execute_piped_commands(commands, cmd_count);

//             // exit(exit_status);
//             printf("[EXECUTING] Executing command: \"%s\"\n", original_cmd);

//             execlp("sh", "sh", "-c", original_cmd, (char *)NULL);
//             perror("execlp");
//             exit(1);

//         } else {
//                 close(pipefd[1]);  // Parent only reads

//                 char output[BUFFER_SIZE * 4] = {0}; // Larger buffer to accumulate all output
//                 char temp[BUFFER_SIZE] = {0};
//                 ssize_t bytes_read;
//                 int total_len = 0;

//                 while ((bytes_read = read(pipefd[0], temp, sizeof(temp) - 1)) > 0) {
//                     memcpy(output + total_len, temp, bytes_read);
//                     total_len += bytes_read;
//                     if (total_len >= sizeof(output)) break; // avoid overflow
//                 }

//                 close(pipefd[0]);

//                 int status;
//                 waitpid(pid, &status, 0);

//                 if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
//                     write(client_fd, output, strlen(output));
//                 } else {
//                     if (total_len == 0) {
//                         const char *msg = "[INFO] Command executed with no output.\n";
//                         write(client_fd, msg, strlen(msg));
//                         printf("[OUTPUT] Sending message to client: \"%s\"\n", msg);

//                     } else {
//                         write(client_fd, output, total_len);
//                         printf("[OUTPUT] Sending output to client:\n%.*s\n", total_len, output);

//                     }
//                 }
//         }
//     }
//     close(client_fd);
//     }
// int main() {
//     int server_fd, client_fd;
//     struct sockaddr_in address;
//     socklen_t addr_len = sizeof(address);

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("[ERROR] Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("[ERROR] Bind failed");
//         exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 3) < 0) {
//         perror("[ERROR] Listen failed");
//         exit(EXIT_FAILURE);
//     }

//     printf("[INFO] Server started. Waiting for client connections on port %d...\n", PORT);

//     while (1) {
//         if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) {
//             perror("[ERROR] Accept failed");
//             continue;
//         }

//         printf("[INFO] Client connected.\n");
//         handle_client(client_fd);
//         printf("[INFO] Ready for next client...\n");
//     }

//     close(server_fd);
//     return 0;
// }


#include "executor.h" // Include the header file for the executor functions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <pthread.h>
#include "client_handler.h"


#define PORT 8080
#define MAX_CMDS 10
#define BUFFER_SIZE 4096

// void *handle_client(void *arg) {
//     int client_fd = *((int *)arg);
//     free(arg); // prevent memory leak
//     char buffer[BUFFER_SIZE] = {0};

//     while (1) {
//         memset(buffer, 0, BUFFER_SIZE);
//         ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
//         if (valread <= 0) {
//             printf("[INFO] Client disconnected.\n");
//             break;
//         }

//         // Strip newline and carriage return
//         buffer[strcspn(buffer, "\r\n")] = '\0';

//         // Validate input
//         int all_whitespace = 1, has_control = 0;
//         for (int i = 0; i < strlen(buffer); i++) {
//             if (buffer[i] < 32 || buffer[i] == 127) has_control = 1;
//             if (buffer[i] != ' ') all_whitespace = 0;
//         }

//         if (strlen(buffer) == 0 || all_whitespace || has_control) {
//             char *err_msg = "Error: Invalid or unsupported input.\n";
//             write(client_fd, err_msg, strlen(err_msg));
//             printf("[WARNING] Ignored invalid input: \"%s\"\n", buffer);
//             continue;
//         }

//         printf("[RECEIVED] Received command: \"%s\" from client.\n", buffer);

//         if (strcmp(buffer, "exit") == 0) {
//             printf("[INFO] Exit command received. Closing client connection.\n");
//             break;
//         }

//         // Parse commands
//         char *commands[MAX_CMDS];
//         int cmd_count = 0;
//         char original_cmd[BUFFER_SIZE];
//         strncpy(original_cmd, buffer, BUFFER_SIZE - 1);
//         original_cmd[BUFFER_SIZE - 1] = '\0';
//         char *cmd = strtok(buffer, "|");
//         int error_detected = 0;

//         while (cmd && cmd_count < MAX_CMDS) {
//             while (*cmd == ' ') cmd++;
//             char *end = cmd + strlen(cmd) - 1;
//             while (end > cmd && *end == ' ') end--;
//             *(end + 1) = '\0';

//             if (strlen(cmd) == 0) {
//                 error_detected = 1;
//                 break;
//             }

//             commands[cmd_count++] = cmd;
//             cmd = strtok(NULL, "|");
//         }

//         if (error_detected || cmd_count == 0 || buffer[0] == '|') {
//             char *error_message;
//             if (buffer[0] == '|') {
//                 error_message = "Error: Missing command before pipe.\n";
//             } else if (cmd_count > 0 && strlen(commands[cmd_count - 1]) == 0) {
//                 error_message = "Error: Missing command after pipe.\n";
//             } else {
//                 error_message = "Error: Empty command between pipes.\n";
//             }

//             fprintf(stderr, "[ERROR] %s", error_message);
//             write(client_fd, error_message, strlen(error_message));
//             printf("[OUTPUT] Sending error message to client: \"%s\"\n", error_message);
//             continue;
//         }

//         // Fork and execute
//         int pipefd[2];
//         if (pipe(pipefd) < 0) {
//             perror("[ERROR] Pipe creation failed");
//             continue;
//         }

//         pid_t pid = fork();
//         if (pid == 0) {
//             // Child
//             dup2(pipefd[1], STDOUT_FILENO);
//             dup2(pipefd[1], STDERR_FILENO);
//             close(pipefd[0]);
//             close(pipefd[1]);

//             printf("[EXECUTING] Executing command: \"%s\"\n", original_cmd);
//             execlp("sh", "sh", "-c", original_cmd, (char *)NULL);
//             perror("execlp");
//             exit(1);
//         } else {
//             // Parent
//             close(pipefd[1]);
//             char output[BUFFER_SIZE * 4] = {0};
//             char temp[BUFFER_SIZE] = {0};
//             ssize_t bytes_read;
//             int total_len = 0;

//             while ((bytes_read = read(pipefd[0], temp, sizeof(temp) - 1)) > 0) {
//                 memcpy(output + total_len, temp, bytes_read);
//                 total_len += bytes_read;
//                 if (total_len >= sizeof(output)) break;
//             }

//             close(pipefd[0]);
//             int status;
//             waitpid(pid, &status, 0);

//             if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
//                 write(client_fd, output, strlen(output));
//             } else {
//                 if (total_len == 0) {
//                     const char *msg = "[INFO] Command executed with no output.\n";
//                     write(client_fd, msg, strlen(msg));
//                     printf("[OUTPUT] Sending message to client: \"%s\"\n", msg);
//                 } else {
//                     write(client_fd, output, total_len);
//                     printf("[OUTPUT] Sending output to client:\n%.*s\n", total_len, output);
//                 }
//             }
//         }
//     }

//     close(client_fd);
//     return NULL;
// }

int main() {
    int client_counter = 0;
    pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
    int server_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[ERROR] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[ERROR] Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("[ERROR] Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Server started. Waiting for client connections on port %d...\n", PORT);

    while (1) {
    //     int *client_fd = malloc(sizeof(int));
    //     *client_fd = accept(server_fd, (struct sockaddr *)&address, &addr_len);

    //     if (*client_fd < 0) {
    //         perror("[ERROR] Accept failed");
    //         free(client_fd);
    //         continue;
    //     }

    //     printf("[INFO] Client connected.\n");

    //     pthread_t tid;
    //     if (pthread_create(&tid, NULL, handle_client, client_fd) != 0) {
    //         perror("[ERROR] Failed to create thread");
    //         close(*client_fd);
    //         free(client_fd);
    //     } else {
    //         pthread_detach(tid);
    //     }
    // }
        client_info *info = malloc(sizeof(client_info));
        info->client_fd = accept(server_fd, (struct sockaddr *)&info->client_addr, &addr_len);

        if (info->client_fd < 0) {
            perror("[ERROR] Accept failed");
            free(info);
            continue;
        }

        // Assign unique client number
        pthread_mutex_lock(&counter_lock);
        info->client_number = ++client_counter;
        pthread_mutex_unlock(&counter_lock);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(info->client_addr.sin_addr), ip, INET_ADDRSTRLEN);
        int port = ntohs(info->client_addr.sin_port);
        printf("[INFO] Client connected: [Client #%d - %s:%d]\n", info->client_number, ip, port);

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, info) != 0) {
            perror("[ERROR] Thread creation failed");
            close(info->client_fd);
            free(info);
        } else {
            pthread_detach(tid);
        }
    }

    close(server_fd);
    return 0;
}
