#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <stdbool.h>

#define MAX_ARGS 100
#define MAX_CMDS 10
#define MAX_PROGRAM_TIME 100

// Command types
typedef enum {
    CMD_SHELL,      // Regular shell command
    CMD_PROGRAM,    // Program simulation command
    CMD_INVALID     // Invalid command
} command_type_t;

// Command structure
typedef struct parsed_command {
    command_type_t type;        // Type of command (shell or program)
    char* original_cmd;         // Original command string
    char** args;               // Array of command arguments
    int program_time;          // Time for program simulation (-1 for shell commands)
    int input_fd;              // Input file descriptor
    int output_fd;             // Output file descriptor
    int error_fd;              // Error file descriptor
    struct parsed_command* next; // Next command in pipeline (NULL if no pipe)
    int pipe_fd[2];            // Pipe file descriptors
} parsed_command_t;

// Parse a command string into a parsed_command_t structure
parsed_command_t* parse_command(const char* cmd);

// Free resources allocated for parsed_command_t
void free_parsed_command(parsed_command_t* cmd);

#endif