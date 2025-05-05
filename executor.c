#include "command_parser.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

// Helper function to read from a file descriptor into a buffer with timeout
static ssize_t read_output(int fd, char *buffer, size_t *current_size, size_t max_size) {
    ssize_t bytes_read;
    size_t remaining = max_size - *current_size;
    
    if (remaining == 0) return 0;
    
    // Make the file descriptor non-blocking
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return -1;
    
    bytes_read = read(fd, buffer + *current_size, remaining);
    
    if (bytes_read > 0) {
        *current_size += bytes_read;
        buffer[*current_size] = '\0';
    } else if (bytes_read == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        return -1;
    }
    
    return bytes_read;
}

static int execute_single_command(parsed_command_t* cmd, char* output, size_t* output_size, size_t max_output_size) {
    if (!cmd || !cmd->args || !cmd->args[0]) {
        fprintf(stderr, "[ERROR] Invalid command structure\n");
        return -1;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {  // Child process
        // Set up input redirection
        if (cmd->input_fd != STDIN_FILENO) {
            dup2(cmd->input_fd, STDIN_FILENO);
            close(cmd->input_fd);
        }

        // Set up output redirection
        if (cmd->output_fd != STDOUT_FILENO) {
            dup2(cmd->output_fd, STDOUT_FILENO);
            close(cmd->output_fd);
        } else {
            dup2(pipefd[1], STDOUT_FILENO);
        }

        // Set up error redirection
        if (cmd->error_fd != STDERR_FILENO) {
            dup2(cmd->error_fd, STDERR_FILENO);
            close(cmd->error_fd);
        }

        close(pipefd[0]);
        close(pipefd[1]);

        execvp(cmd->args[0], cmd->args);
        fprintf(stderr, "[ERROR] Command not found: %s\n", cmd->args[0]);
        exit(127);
    }
    
    // Parent process
    close(pipefd[1]);

    // Close redirected file descriptors in parent
    if (cmd->input_fd != STDIN_FILENO) {
        close(cmd->input_fd);
    }
    if (cmd->output_fd != STDOUT_FILENO) {
        close(cmd->output_fd);
        // Skip reading output if it's redirected to a file
        *output_size = 0;
        output[0] = '\0';
    } else {
        // Only read output if it's not redirected
        *output_size = 0;
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], output + *output_size, max_output_size - *output_size - 1)) > 0) {
            *output_size += bytes_read;
            fprintf(stderr, "[DEBUG] Read %zd bytes of output\n", bytes_read);
        }
        output[*output_size] = '\0';
    }

    if (cmd->error_fd != STDERR_FILENO) {
        close(cmd->error_fd);
    }

    close(pipefd[0]);

    // Wait for child
    int status;
    waitpid(pid, &status, 0);

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int execute_command(char* cmd, char* output, size_t* output_size, size_t max_output_size) {
    fprintf(stderr, "[DEBUG] Executing command: '%s'\n", cmd);
    if (!cmd || !output || !output_size) return -1;
    
    // Initialize output buffer
    *output_size = 0;
    output[0] = '\0';

    parsed_command_t* parsed = parse_command(cmd);
    if (!parsed || parsed->type == CMD_INVALID) {
        fprintf(stderr, "[ERROR] Failed to parse command\n");
        return -1;
    }

    // Check if this is a pipeline
    if (parsed->next) {
        fprintf(stderr, "[DEBUG] Executing pipeline...\n");
        parsed_command_t* current = parsed;
        int result = 0;
        
        while (current && result == 0) {
            result = execute_single_command(current, output, output_size, max_output_size);
            current = current->next;
        }
        
        free_parsed_command(parsed);
        return result;
    }
    
    // Single command
    fprintf(stderr, "[DEBUG] Executing single command...\n");
    int result = execute_single_command(parsed, output, output_size, max_output_size);
    free_parsed_command(parsed);
    return result;
}

int execute_piped_commands(char **commands, int cmd_count, char *output, size_t *output_size, size_t max_output_size) {
    if (!commands || !commands[0] || !output || !output_size) return -1;
    
    // For now, we'll just execute the first command and capture its output
    // TODO: Implement proper pipe chain support with output buffering
    return execute_command(commands[0], output, output_size, max_output_size);
}
