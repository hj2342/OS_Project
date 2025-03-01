#include "command_parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
/*
 * This function parses a given command string into its constituent parts, including
 * the command itself, arguments, and file descriptors for input, output, and error
 * redirection. It populates the provided arrays and pointers with the parsed
 * information.
 */
int parse_command(char *cmd, char **args, int *input_fd, int *output_fd, int *error_fd) {
    char *token = strtok(cmd, " ");
    int i = 0;
    

    while (token != NULL && i < MAX_ARGS - 1) {
        // Check if the token is a quoted string and remove the quotes
        if (token[0] == '"' && token[strlen(token) - 1] == '"') {
            token[strlen(token) - 1] = '\0';  // Remove trailing quote
            token++;  // Move pointer to skip the first quote
        }

        // Check for input redirection
        // Check for input redirection
        if (strcmp(token, "<") == 0) {
            // Move to the next token to get the file name
            token = strtok(NULL, " ");
            // Check if a file name is provided
            if (token == NULL) {
                // Error handling for missing file name
                fprintf(stderr, "Error: No file provided for input redirection.Usage: command < filename\n");
                return -1;
            }
            // Attempt to open the file for reading
            *input_fd = open(token, O_RDONLY);
            // Check if the file was opened successfully
            if (*input_fd == -1) {
                // Error handling for file opening failure
                perror("Error opening input file");
                return -1;
            }
        } 
        // Check for output redirection
        // Check for output redirection
        else if (strcmp(token, ">") == 0) {
            // Move to the next token to get the file name
            token = strtok(NULL, " ");
            // Check if a file name is provided
            if (token == NULL) {
                // Error handling for missing file name
                fprintf(stderr, "Error: No file provided for output redirection.Usage: command > filename\n");
                return -1;
            }
            // Attempt to open the file for writing, creating it if it doesn't exist, truncating it if it does
            *output_fd = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            // Check if the file was opened successfully
            if (*output_fd == -1) {
                // Error handling for file opening failure
                perror("Error creating output file");
                return -1;
            }
        } 
        // Check for error redirection
        else if (strcmp(token, "2>") == 0) {
            // Move to the next token to get the file name
            token = strtok(NULL, " ");
            // Check if a file name is provided
            if (token == NULL) {
                // Error handling for missing file name
                fprintf(stderr, "Error: No file provided for error redirection. Usage: command 2> filename\n");
                return -1;
            }
            // Attempt to open the file for writing, creating it if it doesn't exist, truncating it if it does
            *error_fd = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            // Check if the file was opened successfully
            if (*error_fd == -1) {
                // Error handling for file opening failure
                perror("Error creating error file");
                return -1;
            }
        } 
        // If not a redirection, it's an argument
        else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Null-terminate the arguments array
    return 0;
}