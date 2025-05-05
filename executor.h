#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stddef.h>

// Execute a command and capture its output in a buffer
// Returns: 0 on success, -1 on error
// output: buffer to store command output
// output_size: pointer to store actual output size
// max_output_size: maximum size of output buffer
int execute_command(char *cmd, char *output, size_t *output_size, size_t max_output_size);

// Execute a pipeline of commands
int execute_piped_commands(char **commands, int cmd_count, char *output, size_t *output_size, size_t max_output_size);

#endif