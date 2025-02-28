#include "command_parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int parse_command(char *cmd, char **args, int *input_fd, int *output_fd, int *error_fd) {
    char *token = strtok(cmd, " ");
    int i = 0;
    

    while (token != NULL && i < MAX_ARGS - 1) {
        if (token[0] == '"' && token[strlen(token) - 1] == '"') {
            token[strlen(token) - 1] = '\0';  // Remove trailing quote
            token++;  // Move pointer to skip the first quote
        }

        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for input redirection\n");
                return -1;
            }
            *input_fd = open(token, O_RDONLY);
            if (*input_fd == -1) {
                perror("Error opening input file");
                return -1;
            }
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for output redirection\n");
                return -1;
            }
            *output_fd = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (*output_fd == -1) {
                perror("Error creating output file");
                return -1;
            }
        } else if (strcmp(token, "2>") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for error redirection\n");
                return -1;
            }
            *error_fd = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (*error_fd == -1) {
                perror("Error creating error file");
                return -1;
            }
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
    return 0;
}