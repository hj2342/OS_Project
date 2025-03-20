#include "command_parser.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

int execute_command(char *cmd) {
    // Initialize file descriptors for input, output, and error to -1
    int input_fd = -1, output_fd = -1, error_fd = -1;
    // Array to hold the parsed arguments for the command
    char *args[MAX_ARGS];

    // Attempt to parse the command into arguments and file descriptors
    if (parse_command(cmd, args, &input_fd, &output_fd, &error_fd) == -1) {
        return 127;  // Return 127 if parsing fails
    }

    // Check if the first argument (the command itself) is NULL, indicating an empty command
    if (args[0] == NULL) {
        fprintf(stderr, "Error: Empty command\n");
        return 127;  // Return 127 if args array is empty
    }
    // Check if the command is valid before forking
    // if (access(args[0], X_OK) == -1 && strchr(args[0], '/') == NULL) {
    //     fprintf(stderr, "Error: Command not found: %s\n", args[0]);
    //     return;
    // }

    // Fork a new process
    pid_t pid = fork();

    // Check if fork failed
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    // Child process
    if (pid == 0) {
        // Redirect standard input if input_fd is not -1
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        // Redirect standard output if output_fd is not -1
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        // Redirect standard error if error_fd is not -1
        if (error_fd != -1) {
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
        }

        // Execute the command
        execvp(args[0], args);
           
        fprintf(stderr, "Command not found: %s\n", args[0]);
        exit(127); 
    } else { // Parent process
        // Wait for the child process to finish
        //wait(NULL);
        int status;
        waitpid(pid, &status, 0);
        // Close file descriptors if they were not -1
        if (input_fd != -1) close(input_fd);
        if (output_fd != -1) close(output_fd);
        if (error_fd != -1) close(error_fd);

        if (WIFEXITED(status))
            return WEXITSTATUS(status);
        else
            return 1;
    }
}
// This function is used to execute a series of piped commands
int execute_piped_commands(char **commands, int cmd_count) {
    // Declare arrays to hold the pipe file descriptors and the process IDs
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];

    // Create pipes for each command
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            return 1; // clearly indicate an error
        }
    }

    // Fork a new process for each command
    for (int i = 0; i < cmd_count; i++) {
        if (strlen(commands[i]) == 0) {
            fprintf(stderr, "Error: Empty command between pipes.\n");
            return 1;
        }

        if (cmd_count > 0 && strlen(commands[cmd_count - 1]) == 0) {
            fprintf(stderr, "Error: Missing command after pipe.\n");
            return 1;
        }

        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork failed");
            return 1;
        }

        // Child process
        if (pids[i] == 0) {
            // Initialize file descriptors for input, output, and error to -1
            int input_fd = -1, output_fd = -1, error_fd = -1;
            // Array to hold the parsed arguments for the command
            char *args[MAX_ARGS];

            // Attempt to parse the command into arguments and file descriptors
            parse_command(commands[i], args, &input_fd, &output_fd, &error_fd);

            // Redirect standard input to the read end of the previous pipe if this is not the first command
            if (i > 0) dup2(pipes[i - 1][0], STDIN_FILENO);
            // Redirect standard output to the write end of the current pipe if this is not the last command
            if (i < cmd_count - 1) dup2(pipes[i][1], STDOUT_FILENO);

            // Redirect standard input if input_fd is not -1
            if (input_fd != -1) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            // Redirect standard output if output_fd is not -1
            if (output_fd != -1) {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }
            // Redirect standard error if error_fd is not -1
            if (error_fd != -1) {
                dup2(error_fd, STDERR_FILENO);
                close(error_fd);
            }

            // Close all pipe file descriptors in the child process
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            execvp(args[0], args);
            // If execvp fails, print an error message and exit with status 127
            fprintf(stderr, "Command not found: %s\n", args[0]);
            exit(127);
        }
    }

    // Parent process closes pipes
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Capture and explicitly return the exit status of the last command
    int final_status = 0;
    for (int i = 0; i < cmd_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);

        if (i == cmd_count - 1) { // only last command's status matters
            if (WIFEXITED(status))
                final_status = WEXITSTATUS(status);
            else
                final_status = 1;
        }
    }

    return final_status; // explicitly return final command status
}
