// client_handler.c
#include "client_handler.h"
#include "executor.h"
#include "queue.h"
#include "command_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SIZE 4096 // Size of the buffer for reading and writing data
#define MAX_CMDS 10 // Maximum number of commands that can be piped

// Global request queue pointer
static RequestQueue* g_request_queue = NULL;

// Initialize client handler with request queue
void init_client_handler(RequestQueue* queue) {
    g_request_queue = queue;
}

// Function to handle client connections
void* handle_client(void* arg) {
    // Cast the argument to client_info structure
    client_info* info = (client_info*)arg;
    int client_fd = info->client_fd;
    int client_number = info->client_number;
    struct sockaddr_in client_addr = info->client_addr;

    // Convert IP and port to readable strings
    char ip[INET_ADDRSTRLEN]; // Buffer to hold the IP address
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN); // Convert IP to string
    int port = ntohs(client_addr.sin_port); // Convert port number to host byte order

    // Free memory allocated in main for client_info structure
    free(info);

    char buffer[BUFFER_SIZE] = {0}; // Buffer to hold incoming data

    while (1) {
        // Clear the buffer for each iteration
        memset(buffer, 0, BUFFER_SIZE);
        // Read data from the client
        ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
        // Check if the client has disconnected
        if (valread <= 0) {
            printf("[INFO] [Client #%d - %s:%d] Client disconnected.\n", client_number, ip, port);
            break;
        }

        // Remove newline characters from the buffer
        buffer[strcspn(buffer, "\r\n")] = '\0';

        // Check if the buffer contains only whitespace or control characters
        int all_whitespace = 1, has_control = 0;
        for (size_t i = 0; i < strlen(buffer); i++) {
            if (buffer[i] < 32 || buffer[i] == 127) has_control = 1; // Check for control characters
            if (buffer[i] != ' ') all_whitespace = 0; // Check for non-whitespace characters
        }

        // Handle invalid or unsupported input
        if (strlen(buffer) == 0 || all_whitespace || has_control) {
            const char *err_msg = "Error: Invalid or unsupported input.\n";
            write(client_fd, err_msg, strlen(err_msg));
            printf("[WARNING] [Client #%d - %s:%d] Ignored invalid input: \"%s\"\n", client_number, ip, port, buffer);
            continue;
        }

        // Log received command
        printf("[RECEIVED] [Client #%d - %s:%d] Received command: \"%s\"\n", client_number, ip, port, buffer);

        // Handle exit command
        if (strcmp(buffer, "exit") == 0) {
            printf("[INFO] [Client #%d - %s:%d] Exit command received. Closing connection.\n", client_number, ip, port);
            break;
        }

        // Parse the command
        fprintf(stderr, "[DEBUG] Client handler parsing command: '%s'\n", buffer);
        parsed_command_t* parsed_cmd = parse_command(buffer);
        if (!parsed_cmd || parsed_cmd->type == CMD_INVALID) {
            fprintf(stderr, "[DEBUG] Command parsing failed\n");
            const char* error_msg = "[ERROR] Failed to parse command due to memory allocation\n";
            write(client_fd, error_msg, strlen(error_msg));
            fprintf(stderr, "[ERROR] [Client #%d - %s:%d] %s", client_number, ip, port, error_msg);
            continue;
        }

        // Log the parsed command type
        printf("[INFO] [Client #%d - %s:%d] Parsed command type: %s\n", 
               client_number, ip, port,
               parsed_cmd->type == CMD_SHELL ? "Shell Command" : "Program Simulation");

        // Create and initialize request
        fprintf(stderr, "[DEBUG] Creating new request\n");
        Request* request = create_request(
            client_fd,
            parsed_cmd->original_cmd,
            parsed_cmd->type == CMD_SHELL,
            parsed_cmd->type == CMD_PROGRAM ? parsed_cmd->program_time : -1
        );

        if (!request) {
            fprintf(stderr, "[ERROR] Failed to allocate request\n");
            const char* error_msg = "[ERROR] Failed to create request due to memory allocation\n";
            write(client_fd, error_msg, strlen(error_msg));
            fprintf(stderr, "[ERROR] [Client #%d - %s:%d] %s", client_number, ip, port, error_msg);
            free_parsed_command(parsed_cmd);
            continue;
        }

        // Enqueue the request
        fprintf(stderr, "[DEBUG] Enqueueing request to global queue\n");
        if (!enqueue(g_request_queue, request)) {
            const char* error_msg = "[ERROR] Failed to enqueue request (queue might be full)\n";
            write(client_fd, error_msg, strlen(error_msg));
            fprintf(stderr, "[ERROR] [Client #%d - %s:%d] %s", client_number, ip, port, error_msg);
            destroy_request(request);
            free_parsed_command(parsed_cmd);
            continue;
        }
        fprintf(stderr, "[DEBUG] Queue size after enqueue: %d\n", g_request_queue->size);
        fprintf(stderr, "[DEBUG] Signaling scheduler\n");
        pthread_cond_signal(&g_request_queue->not_empty);

        // Log successful enqueue with command details
        if (parsed_cmd->type == CMD_PROGRAM) {
            printf("[INFO] [Client #%d - %s:%d] Enqueued program simulation request (time: %d seconds)\n",
                   client_number, ip, port, parsed_cmd->program_time);
        } else {
            printf("[INFO] [Client #%d - %s:%d] Enqueued shell command: \"%s\"\n",
                   client_number, ip, port, parsed_cmd->original_cmd);
        }

        free_parsed_command(parsed_cmd);
    }

    // Close the client file descriptor
    close(client_fd);
    return NULL;
}
