
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_CMDS 10
int main() {
    // Initialize variables for reading input
    char *input = NULL;
    size_t len = 0;

    // Main loop to continuously read and execute commands
    while (1) {
        printf("$ ");
        // Read a line of input
        ssize_t line = getline(&input, &len, stdin);
        if (line == -1) {
            perror("Error reading input");
            exit(1);
        }

        // Remove the newline character from the end of the input
        input[strcspn(input, "\n")] = 0; 

        // Check if the user wants to exit
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Initialize variables for parsing commands
        char *commands[MAX_CMDS];
        int cmd_count = 0;
        // Split the input into individual commands separated by pipes
        char *cmd = strtok(input, "|");

        // Loop through each command
        while (cmd != NULL && cmd_count < MAX_CMDS) {
            // Remove leading and trailing spaces from the command
            while (*cmd == ' ') cmd++;
            char *end = cmd + strlen(cmd) - 1;
            while (end > cmd && *end == ' ') end--;
            *(end + 1) = 0;

            // Check if the command is not empty
            if (strlen(cmd) > 0) {
                commands[cmd_count++] = cmd;
            } else {
                fprintf(stderr, "Error: Empty command between pipes\n");
                cmd_count = 0;
                break;
            }
            // Move to the next command
            cmd = strtok(NULL, "|");
        }

        // Skip if no valid commands were found
        if (cmd_count == 0) {
            continue;
        }

        // Execute the command(s)
        if (cmd_count == 1) {
            execute_command(commands[0]);
        } else {
            execute_piped_commands(commands, cmd_count);
        }
    }

    // Free dynamically allocated memory for input
    free(input);
    return 0;
}
