// client_handler.c
#include "client_handler.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096
#define MAX_CMDS 10

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);
    char buffer[BUFFER_SIZE] = {0};

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (valread <= 0) {
            printf("[INFO] Client disconnected.\n");
            break;
        }

        buffer[strcspn(buffer, "\r\n")] = '\0';

        int all_whitespace = 1, has_control = 0;
        for (int i = 0; i < strlen(buffer); i++) {
            if (buffer[i] < 32 || buffer[i] == 127) has_control = 1;
            if (buffer[i] != ' ') all_whitespace = 0;
        }

        if (strlen(buffer) == 0 || all_whitespace || has_control) {
            const char *err_msg = "Error: Invalid or unsupported input.\n";
            write(client_fd, err_msg, strlen(err_msg));
            printf("[WARNING] Ignored invalid input: \"%s\"\n", buffer);
            continue;
        }

        printf("[RECEIVED] Received command: \"%s\" from client.\n", buffer);

        if (strcmp(buffer, "exit") == 0) {
            printf("[INFO] Exit command received. Closing client connection.\n");
            break;
        }

        char *commands[MAX_CMDS];
        int cmd_count = 0;
        char original_cmd[BUFFER_SIZE];
        strncpy(original_cmd, buffer, BUFFER_SIZE - 1);
        original_cmd[BUFFER_SIZE - 1] = '\0';
        char *cmd = strtok(buffer, "|");
        int error_detected = 0;

        while (cmd && cmd_count < MAX_CMDS) {
            while (*cmd == ' ') cmd++;
            char *end = cmd + strlen(cmd) - 1;
            while (end > cmd && *end == ' ') end--;
            *(end + 1) = '\0';

            if (strlen(cmd) == 0) {
                error_detected = 1;
                break;
            }

            commands[cmd_count++] = cmd;
            cmd = strtok(NULL, "|");
        }

        if (error_detected || cmd_count == 0 || buffer[0] == '|') {
            const char *error_message;
            if (buffer[0] == '|') {
                error_message = "Error: Missing command before pipe.\n";
            } else if (cmd_count > 0 && strlen(commands[cmd_count - 1]) == 0) {
                error_message = "Error: Missing command after pipe.\n";
            } else {
                error_message = "Error: Empty command between pipes.\n";
            }

            fprintf(stderr, "[ERROR] %s", error_message);
            write(client_fd, error_message, strlen(error_message));
            printf("[OUTPUT] Sending error message to client: \"%s\"\n", error_message);
            continue;
        }

        int pipefd[2];
        if (pipe(pipefd) < 0) {
            perror("[ERROR] Pipe creation failed");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            printf("[EXECUTING] Executing command: \"%s\"\n", original_cmd);
            execlp("sh", "sh", "-c", original_cmd, (char *)NULL);
            perror("execlp");
            exit(1);
        } else {
            close(pipefd[1]);
            char output[BUFFER_SIZE * 4] = {0};
            char temp[BUFFER_SIZE] = {0};
            ssize_t bytes_read;
            int total_len = 0;

            while ((bytes_read = read(pipefd[0], temp, sizeof(temp) - 1)) > 0) {
                memcpy(output + total_len, temp, bytes_read);
                total_len += bytes_read;
                if (total_len >= sizeof(output)) break;
            }

            close(pipefd[0]);
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
                write(client_fd, output, strlen(output));
            } else {
                if (total_len == 0) {
                    const char *msg = "[INFO] Command executed with no output.\n";
                    write(client_fd, msg, strlen(msg));
                    printf("[OUTPUT] Sending message to client: \"%s\"\n", msg);
                } else {
                    write(client_fd, output, total_len);
                    printf("[OUTPUT] Sending output to client:\n%.*s\n", total_len, output);
                }
            }
        }
    }

    close(client_fd);
    return NULL;
}
