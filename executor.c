#include "command_parser.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

void execute_command(char *cmd) {
    // Initialize file descriptors for input, output, and error to -1
    int input_fd = -1, output_fd = -1, error_fd = -1;
    // Array to hold the parsed arguments for the command
    char *args[MAX_ARGS];

    // Attempt to parse the command into arguments and file descriptors
    if (parse_command(cmd, args, &input_fd, &output_fd, &error_fd) == -1) {
        return;  // If parsing fails, do not execute the command
    }

    // Check if the first argument (the command itself) is NULL, indicating an empty command
    if (args[0] == NULL) {
        fprintf(stderr, "Error: Empty command\n");
        return;
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
        // If execvp fails, print the error and exit
        perror("execvp failed");
        exit(1);
    } else { // Parent process
        // Wait for the child process to finish
        wait(NULL);
        // Close file descriptors if they were not -1
        if (input_fd != -1) close(input_fd);
        if (output_fd != -1) close(output_fd);
        if (error_fd != -1) close(error_fd);
    }
}
void execute_piped_commands(char **commands, int cmd_count) {
    // Array to hold the file descriptors for the pipes
    int pipes[MAX_CMDS - 1][2];
    // Array to hold the process IDs of the child processes
    pid_t pids[MAX_CMDS];

    // Create pipes for communication between processes
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed"); // Handle error if pipe creation fails
            exit(1); // Exit if pipe creation fails
        }
    }

    // Iterate through each command to execute
    for (int i = 0; i < cmd_count; i++) {
        // Check if the current command is empty
        if(strlen(commands[i]) == 0){
            fprintf(stderr, "Error: Empty command between pipes.\n"); // Error message for empty command
            return; // Exit if an empty command is found
        }
        // Check if the last command is missing after the last pipe
        if (cmd_count > 0 && strlen(commands[cmd_count - 1]) == 0) {
            fprintf(stderr, "Error: Missing command after pipe.\n"); // Error message for missing command after pipe
            return; // Exit if a command is missing after the last pipe
        }
        
        // Fork a new process for each command
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork failed"); // Handle error if fork fails
            exit(1); // Exit if fork fails
        }

        if (pids[i] == 0) { // Child process
            // Variables to hold file descriptors for input, output, and error
            int input_fd = -1, output_fd = -1, error_fd = -1;
            // Array to hold the parsed arguments for the command
            char *args[MAX_ARGS];

            // Parse the current command into arguments and file descriptors
            parse_command(commands[i], args, &input_fd, &output_fd, &error_fd);
            

            // Redirect standard input from the previous pipe if not the first command
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            // Redirect standard output to the next pipe if not the last command
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // Redirect file descriptors for input, output, and error if specified
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

            // Close all pipe file descriptors in the child process
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            execvp(args[0], args);
            perror("execvp failed"); // Handle error if execvp fails
            exit(1); // Exit if execvp fails
        }
    }

    // Close all pipe file descriptors in the parent process
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < cmd_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}
