#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#define MAX_ARGS 100
#define MAX_CMDS 10

void parse_command(char *cmd, char **args, int *input_fd, int *output_fd, int *error_fd);

#endif