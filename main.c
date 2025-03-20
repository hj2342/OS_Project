
// #include "executor.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <netinet/in.h>
// #define PORT 8080
// #define MAX_CMDS 10
// #define BUFFER_SIZE 4096
// int main() {


//     // Initialize variables for reading input
//     char *input = NULL;
//     size_t len = 0;

//     // Main loop to continuously read and execute commands
//     while (1) {
//         printf("$ ");
//         // Read a line of input
//         ssize_t line = getline(&input, &len, stdin);
//         if (line == -1) {
//             perror("Error reading input");
//             exit(1);
//         }

//         // Remove the newline character from the end of the input
//         input[strcspn(input, "\n")] = 0; 

//         // Check if the user wants to exit
//         if (strcmp(input, "exit") == 0) {
//             break;
//         }

//         // Initialize variables for parsing commands
//         char *commands[MAX_CMDS];
//         int cmd_count = 0;
//         // Split the input into individual commands separated by pipes
//         char *cmd = strtok(input, "|");

//         // Loop through each command
//         while (cmd != NULL && cmd_count < MAX_CMDS) {
//             // Remove leading and trailing spaces from the command
//             while (*cmd == ' ') cmd++;
//             char *end = cmd + strlen(cmd) - 1;
//             while (end > cmd && *end == ' ') end--;
//             *(end + 1) = 0;

//             // Check if the command is not empty
//             if (strlen(cmd) > 0) {
//                 commands[cmd_count++] = cmd;
//             } else {
//                 fprintf(stderr, "Error: Empty command between pipes\n");
//                 cmd_count = 0;
//                 break;
//             }
//             // Move to the next command
//             cmd = strtok(NULL, "|");
//         }

//         // Skip if no valid commands were found
//         if (cmd_count == 0) {
//             continue;
//         }

//         // Execute the command(s)
//         if (cmd_count == 1) {
//             execute_command(commands[0]);
//         } else {
//             execute_piped_commands(commands, cmd_count);
//         }
//     }

//     // Free dynamically allocated memory for input
//     free(input);
//     return 0;
// }

#include "executor.h" // Include the header file for the executor functions
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <netinet/in.h> 
#include <sys/wait.h> 

#define PORT 8080 // Define the port number for the server
#define MAX_CMDS 10 // Define the maximum number of commands that can be piped
#define BUFFER_SIZE 4096 // Define the size of the buffer for reading and writing data

int main() {
    int server_fd, client_fd; // Declare variables for server and client file descriptors
    struct sockaddr_in address; // Declare a structure for the server address
    socklen_t addr_len = sizeof(address); // Declare and initialize the length of the server address structure

    // Socket setup
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { // Attempt to create a socket
        perror("[ERROR] Socket creation failed"); // Print error message if socket creation fails
        exit(EXIT_FAILURE); // Exit the program if socket creation fails
    }

    address.sin_family = AF_INET; // Set the address family to Internet
    address.sin_addr.s_addr = INADDR_ANY; // Set the address to accept connections on all network interfaces
    address.sin_port = htons(PORT); // Set the port number for the server

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { // Attempt to bind the socket to the address and port
        perror("[ERROR] Bind failed"); // Print error message if binding fails
        exit(EXIT_FAILURE); // Exit the program if binding fails
    }

    if (listen(server_fd, 3) < 0) { // Attempt to listen for incoming connections
        perror("[ERROR] Listen failed"); // Print error message if listening fails
        exit(EXIT_FAILURE); // Exit the program if listening fails
    }

    printf("[INFO] Server started, waiting for client connections...\n"); // Print message indicating server is running and waiting for connections

    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addr_len)) < 0) { // Attempt to accept an incoming connection
        perror("[ERROR] Accept failed"); // Print error message if accepting a connection fails
        exit(EXIT_FAILURE); // Exit the program if accepting a connection fails
    }

    printf("[INFO] Client connected.\n"); // Print message indicating a client has connected

    char buffer[BUFFER_SIZE] = {0}; // Declare and initialize a buffer for reading data

    while (1) {
        memset(buffer, 0, BUFFER_SIZE); // Clear the buffer before each read operation

        ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1); // Attempt to read data from the client
        if (valread <= 0) { // Check if the read operation was successful
            printf("[INFO] Client disconnected.\n"); // Print message indicating the client has disconnected
            break; // Exit the loop if the client has disconnected
        }

        buffer[strcspn(buffer, "\n")] = '\0'; // Remove the newline character from the end of the buffer
        printf("[RECEIVED] Received command: \"%s\" from client.\n", buffer); // Print the received command

        if (strcmp(buffer, "exit") == 0) { // Check if the received command is "exit"
            printf("[INFO] Exit command received. Closing connection.\n"); // Print message indicating the exit command was received
            break; // Exit the loop if the exit command was received
        }

        char *commands[MAX_CMDS]; // Declare an array to hold the parsed commands
        int cmd_count = 0; // Initialize the command count to 0
        char *cmd = strtok(buffer, "|"); // Split the buffer into individual commands separated by pipes
        int error_detected = 0; // Flag to detect errors in command parsing

        while (cmd && cmd_count < MAX_CMDS) { // Loop through each command
            // Trim whitespace from the command
            while (*cmd == ' ') cmd++;
            char *end = cmd + strlen(cmd) - 1;
            while (end > cmd && *end == ' ') end--;
            *(end + 1) = '\0';

            // Check if the command is not empty after trimming
            if (strlen(cmd) == 0) {
                error_detected = 1; // Set the error flag if an empty command is found
                break; 
            }

            commands[cmd_count++] = cmd; // Add the command to the array if it's not empty
            cmd = strtok(NULL, "|"); // Move to the next command
        }

        // Handle errors detected during command parsing
        if (error_detected || cmd_count == 0 || buffer[0] == '|') {
            char *error_message; // Declare a pointer to hold the error message

            // Determine the appropriate error message based on the error condition
            if (buffer[0] == '|') {
                error_message = "Error: Missing command before pipe.\n";
            } else if (cmd_count > 0 && strlen(commands[cmd_count - 1]) == 0) {
                error_message = "Error: Missing command after pipe.\n";
            } else {
                error_message = "Error: Empty command between pipes.\n";
            }

            fprintf(stderr, "[ERROR] %s", error_message); // Print the error message to the standard error
            write(client_fd, error_message, strlen(error_message)); // Send the error message to the client
            printf("[OUTPUT] Sending error message to client: \"%s\"\n", error_message); 
            continue; // Skip to the next iteration of the loop
        }

        if (cmd_count == 0) continue; // Skip to the next iteration if no valid commands were found

        printf("[EXECUTING] Executing command: \"%s\"\n", buffer); // Print message indicating the command is being executed

        // Setup pipe to capture both stdout and stderr dynamically
        int pipefd[2]; // Declare an array to hold the pipe file descriptors
        if (pipe(pipefd) < 0) { // Attempt to create a pipe
            perror("[ERROR] Pipe creation failed"); // Print error message if pipe creation fails
            continue; // Skip to the next iteration if pipe creation fails
        }

        pid_t pid = fork(); // Attempt to fork a new process

        if (pid == 0) { // Child process
            // Redirect stdout and stderr to the pipe
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            int exit_status; // Declare a variable to hold the exit status of the command

            // Execute the command(s) based on the number of commands
            if (cmd_count == 1) {
                exit_status = execute_command(commands[0]);
            } else {
                exit_status = execute_piped_commands(commands, cmd_count);
            }

            exit(exit_status); // Exit the child process with the appropriate status
        } else { // Parent process
            // Parent process: capture output and error dynamically
            close(pipefd[1]); // Close the write end of the pipe

            int status; // Declare a variable to hold the status of the child process
            waitpid(pid, &status, 0); // Wait for the child process to finish

            char output[BUFFER_SIZE] = {0}; // Declare a buffer to hold the output
            ssize_t output_len = read(pipefd[0], output, BUFFER_SIZE - 1); // Attempt to read the output from the pipe
            close(pipefd[0]); // Close the read end of the pipe

            // Dynamically handle errors based on the child's exit status
            if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
                printf("[ERROR] %s\n", output); // Print the error message if the child process exited with status 127
                write(client_fd, output, strlen(output)); // Send the error message to the client
                printf("[OUTPUT] Sending error message to client: \"%s\"\n", output); // Print message indicating an error message was sent to the client
            } else {
                write(client_fd, output, output_len); // Send the output to the client
                printf("[OUTPUT] Sending output to client:\n%s\n", output); // Print message indicating output was sent to the client
            }
        }
    }

    // Cleanup
    close(client_fd); // Close the client file descriptor
    close(server_fd); // Close the server file descriptor

    return 0; 
}