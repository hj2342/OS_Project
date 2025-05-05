// Include necessary header files
#include "executor.h"
#include "client_handler.h"
#include "queue.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>


// Define constants for the port number, maximum number of commands, and buffer size
#define PORT 8080
#define MAX_CMDS 10
#define BUFFER_SIZE 4096


int main() {
    // Initialize a counter for client connections and a mutex for thread-safe access
    int client_counter = 0;
    pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

    // Create request queue
    RequestQueue* request_queue = create_queue();
    if (!request_queue) {
        fprintf(stderr, "[ERROR] Failed to create request queue\n");
        exit(EXIT_FAILURE);
    }

    // Initialize client handler with request queue
    init_client_handler(request_queue);

    // Initialize scheduler
    if (init_scheduler(request_queue) != 0) {
        fprintf(stderr, "[ERROR] Failed to initialize scheduler\n");
        destroy_queue(request_queue);
        exit(EXIT_FAILURE);
    }
    // Create a socket for the server
    int server_fd;
    // Define the server address structure
    struct sockaddr_in address;
    // Size of the server address structure
    socklen_t addr_len = sizeof(address);

    // Attempt to create a socket for the server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[ERROR] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure the server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Attempt to bind the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[ERROR] Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 10) < 0) {
        perror("[ERROR] Listen failed");
        exit(EXIT_FAILURE);
    }

    // Inform the user that the server is running and waiting for connections
    printf("[INFO] Server started. Waiting for client connections on port %d...\n", PORT);

    // Main server loop to accept and handle client connections
    while (1) {

        // Allocate memory for client information
        client_info *info = malloc(sizeof(client_info));
        // Accept an incoming connection
        info->client_fd = accept(server_fd, (struct sockaddr *)&info->client_addr, &addr_len);

        // Check if the accept operation failed
        if (info->client_fd < 0) {
            perror("[ERROR] Accept failed");
            free(info);
            continue;
        }

        // Assign a unique number to the client
        pthread_mutex_lock(&counter_lock);
        info->client_number = ++client_counter;
        pthread_mutex_unlock(&counter_lock);

        // Convert the client's IP address and port to a readable format
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(info->client_addr.sin_addr), ip, INET_ADDRSTRLEN);
        int port = ntohs(info->client_addr.sin_port);
        // Log the client connection
        printf("[INFO] Client connected: [Client #%d - %s:%d]\n", info->client_number, ip, port);

        // Create a new thread to handle the client connection
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, info) != 0) {
            perror("[ERROR] Thread creation failed");
            close(info->client_fd);
            free(info);
        } else {
            // Detach the thread to allow it to run independently
            pthread_detach(tid);
        }
    }

    // Close the server socket and cleanup
    close(server_fd);
    destroy_queue(request_queue);
    return 0;
}
