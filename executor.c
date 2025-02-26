#include "command_parser.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

void execute_command(char *cmd) {
    int input_fd = -1, output_fd = -1, error_fd = -1;
    char *args[MAX_ARGS];

    parse_command(cmd, args, &input_fd, &output_fd, &error_fd);

    if (args[0] == NULL) {
        fprintf(stderr, "Error: Empty command\n");
        return;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) { // Child process
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        if (error_fd != -1) {
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
        }

        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else { // Parent process
        wait(NULL);
        if (input_fd != -1) close(input_fd);
        if (output_fd != -1) close(output_fd);
        if (error_fd != -1) close(error_fd);
    }
}

void execute_piped_commands(char **commands, int cmd_count) {
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];

    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            exit(1);
        }
    }

    for (int i = 0; i < cmd_count; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork failed");
            exit(1);
        }

        if (pids[i] == 0) { // Child process
            int input_fd = -1, output_fd = -1, error_fd = -1;
            char *args[MAX_ARGS];

            parse_command(commands[i], args, &input_fd, &output_fd, &error_fd);

            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            if (input_fd != -1) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (output_fd != -1) {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }
            if (error_fd != -1) {
                dup2(error_fd, STDERR_FILENO);
                close(error_fd);
            }

            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        }
    }

    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < cmd_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}