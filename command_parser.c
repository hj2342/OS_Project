
#include "command_parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

// Helper function to check if a string is a valid number
static bool is_valid_number(const char* str) {
    if (!str || !*str) return false;
    
    // Skip leading whitespace
    while (isspace(*str)) str++;
    
    // Handle optional sign
    if (*str == '+' || *str == '-') str++;
    
    // Must have at least one digit
    if (!isdigit(*str)) return false;
    
    // Check remaining characters
    while (*str) {
        if (!isdigit(*str)) return false;
        str++;
    }
    
    return true;
}

// Helper function to parse program time
static int parse_program_time(const char* cmd) {
    // Skip "program" keyword and any whitespace
    const char* ptr = cmd + 7;  // Length of "program"
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Check if we have a valid number
    if (!is_valid_number(ptr)) return -1;
    
    // Convert to integer
    int time = atoi(ptr);
    if (time <= 0 || time > MAX_PROGRAM_TIME) return -1;
    
    return time;
}

// Helper function to allocate and copy a string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* dup = strdup(str);
    if (!dup) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    return dup;
}

// Helper function to parse a single command (no pipes)
static parsed_command_t* parse_single_command(const char* cmd, char** next_cmd) {
    if (!cmd) return NULL;

    // Find pipe symbol if it exists
    char* pipe_pos = strchr(cmd, '|');
    if (pipe_pos) {
        *pipe_pos = '\0';  // Temporarily terminate string at pipe
        *next_cmd = pipe_pos + 1;
    } else {
        *next_cmd = NULL;
    }

    // Allocate memory for the command structure
    parsed_command_t* result = calloc(1, sizeof(parsed_command_t));
    if (!result) {
        perror("Memory allocation failed");
        return NULL;
    }

    // Initialize file descriptors
    result->input_fd = STDIN_FILENO;
    result->output_fd = STDOUT_FILENO;
    result->error_fd = STDERR_FILENO;

    // Save original command
    result->original_cmd = strdup_safe(cmd);

    // Skip leading whitespace
    while (*cmd && isspace(*cmd)) cmd++;

    // Check for empty command
    if (!*cmd) {
        result->type = CMD_INVALID;
        return result;
    }

    // Check if this is a program simulation command
    if (strncmp(cmd, "program", 7) == 0 && (isspace(cmd[7]) || cmd[7] == '\0')) {
        result->type = CMD_PROGRAM;
        result->program_time = parse_program_time(cmd);
        if (result->program_time == -1) {
            result->type = CMD_INVALID;
        }
        return result;
    }

    // This is a shell command
    result->type = CMD_SHELL;
    result->program_time = -1;

    // Allocate space for arguments
    result->args = calloc(MAX_ARGS, sizeof(char*));
    if (!result->args) {
        free_parsed_command(result);
        return NULL;
    }

    // Make a copy of the command for tokenization
    char* cmd_copy = strdup_safe(cmd);
    char* token = strtok(cmd_copy, " \t\n");
    int i = 0;

    while (token && i < MAX_ARGS - 1) {
        // Handle input redirection
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                fprintf(stderr, "Error: No file provided for input redirection\n");
                free(cmd_copy);
                free_parsed_command(result);
                return NULL;
            }
            result->input_fd = open(token, O_RDONLY);
            if (result->input_fd == -1) {
                perror("Error opening input file");
                free(cmd_copy);
                free_parsed_command(result);
                return NULL;
            }
        }
        // Handle output redirection
        else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                fprintf(stderr, "Error: No file provided for output redirection\n");
                free(cmd_copy);
                free_parsed_command(result);
                return NULL;
            }
            result->output_fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (result->output_fd == -1) {
                perror("Error opening output file");
                free(cmd_copy);
                free_parsed_command(result);
                return NULL;
            }
        }
        // Handle error redirection
        else if (strcmp(token, "2>") == 0) {
            token = strtok(NULL, " ");
            if (!token) {
                fprintf(stderr, "Error: No file provided for error redirection\n");
                free(cmd_copy);
                free_parsed_command(result);
                return NULL;
            }
            result->error_fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (result->error_fd == -1) {
                perror("Error opening error file");
                free(cmd_copy);
                free_parsed_command(result);
                return NULL;
            }
        }
        // Regular argument
        else {
            result->args[i++] = strdup_safe(token);
        }
        token = strtok(NULL, " ");
    }

    result->args[i] = NULL;
    free(cmd_copy);
    return result;
}

// Parse a command string into a parsed_command_t structure
parsed_command_t* parse_command(const char* cmd) {
    if (!cmd) return NULL;

    char* next_cmd;
    parsed_command_t* first = parse_single_command(cmd, &next_cmd);
    if (!first) return NULL;

    parsed_command_t* current = first;
    while (next_cmd) {
        // Skip leading whitespace in next command
        while (*next_cmd && isspace(*next_cmd)) next_cmd++;
        if (!*next_cmd) break;

        // Create pipe for current command
        if (pipe(current->pipe_fd) == -1) {
            perror("Failed to create pipe");
            free_parsed_command(first);
            return NULL;
        }

        // Set output of current command to pipe write end
        current->output_fd = current->pipe_fd[1];

        // Parse next command in pipeline
        current->next = parse_single_command(next_cmd, &next_cmd);
        if (!current->next) {
            free_parsed_command(first);
            return NULL;
        }

        // Set input of next command to pipe read end
        current->next->input_fd = current->pipe_fd[0];

        current = current->next;
    }

    return first;
}

// Free resources allocated for parsed_command_t
void free_parsed_command(parsed_command_t* cmd) {
    if (!cmd) return;

    // First free the next command in the pipeline
    if (cmd->next) {
        free_parsed_command(cmd->next);
    }

    // Close pipe file descriptors if they exist
    if (cmd->pipe_fd[0] != 0) close(cmd->pipe_fd[0]);
    if (cmd->pipe_fd[1] != 0) close(cmd->pipe_fd[1]);

    free(cmd->original_cmd);

    if (cmd->args) {
        for (int i = 0; cmd->args[i]; i++) {
            free(cmd->args[i]);
        }
        free(cmd->args);
    }

    // Close file descriptors if they were opened
    if (cmd->input_fd != STDIN_FILENO) close(cmd->input_fd);
    if (cmd->output_fd != STDOUT_FILENO) close(cmd->output_fd);
    if (cmd->error_fd != STDERR_FILENO) close(cmd->error_fd);

    free(cmd);
}