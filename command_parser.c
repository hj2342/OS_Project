#include "command_parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void parse_command(char *cmd, char **args, int *input_fd, int *output_fd, int *error_fd) {
    char *token = strtok(cmd, " ");
    int i = 0;

    while (token != NULL && i < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for input redirection\n");
                return;
            }
            *input_fd = open(token, O_RDONLY);
            if (*input_fd == -1) {
                perror("Error opening input file");
                return;
            }
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for output redirection\n");
                return;
            }
            *output_fd = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (*output_fd == -1) {
                perror("Error creating output file");
                return;
            }
        } else if (strcmp(token, "2>") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for error redirection\n");
                return;
            }
            *error_fd = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (*error_fd == -1) {
                perror("Error creating error file");
                return;
            }
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}