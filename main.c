// // Include the necessary header files for the executor functions, standard input/output, standard library, string manipulation, socket operations, and thread operations
// #include "executor.h" // Include the header file for the executor functions
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <netinet/in.h>
// #include <sys/wait.h>
// #include <pthread.h>
// #include "client_handler.h"


// // Define constants for the port number, maximum number of commands, and buffer size
// #define PORT 8080
// #define MAX_CMDS 10
// #define BUFFER_SIZE 4096


// int main() {
//     // Initialize a counter for client connections and a mutex for thread-safe access
//     int client_counter = 0;
//     pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
//     // Create a socket for the server
//     int server_fd;
//     // Define the server address structure
//     struct sockaddr_in address;
//     // Size of the server address structure
//     socklen_t addr_len = sizeof(address);

//     // Attempt to create a socket for the server
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("[ERROR] Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     // Configure the server address
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     // Attempt to bind the socket to the specified address and port
//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("[ERROR] Bind failed");
//         exit(EXIT_FAILURE);
//     }

//     // Listen for incoming connections
//     if (listen(server_fd, 10) < 0) {
//         perror("[ERROR] Listen failed");
//         exit(EXIT_FAILURE);
//     }

//     // Inform the user that the server is running and waiting for connections
//     printf("[INFO] Server started. Waiting for client connections on port %d...\n", PORT);

//     // Main server loop to accept and handle client connections
//     while (1) {

//         // Allocate memory for client information
//         client_info *info = malloc(sizeof(client_info));
//         // Accept an incoming connection
//         info->client_fd = accept(server_fd, (struct sockaddr *)&info->client_addr, &addr_len);

//         // Check if the accept operation failed
//         if (info->client_fd < 0) {
//             perror("[ERROR] Accept failed");
//             free(info);
//             continue;
//         }

//         // Assign a unique number to the client
//         pthread_mutex_lock(&counter_lock);
//         info->client_number = ++client_counter;
//         pthread_mutex_unlock(&counter_lock);

//         // Convert the client's IP address and port to a readable format
//         char ip[INET_ADDRSTRLEN];
//         inet_ntop(AF_INET, &(info->client_addr.sin_addr), ip, INET_ADDRSTRLEN);
//         int port = ntohs(info->client_addr.sin_port);
//         // Log the client connection
//         printf("[INFO] Client connected: [Client #%d - %s:%d]\n", info->client_number, ip, port);

//         // Create a new thread to handle the client connection
//         pthread_t tid;
//         if (pthread_create(&tid, NULL, handle_client, info) != 0) {
//             perror("[ERROR] Thread creation failed");
//             close(info->client_fd);
//             free(info);
//         } else {
//             // Detach the thread to allow it to run independently
//             pthread_detach(tid);
//         }
//     }

//     // Close the server socket
//     close(server_fd);
//     return 0;
// }
// main.c


// main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include "client_handler.h"
#include "scheduler.h"
#include "task.h"           // Include task.h for Task structure
#include "queue.h"          // Include queue.h for queue operations
#include "task_executor.h"  // Include task_executor.h for execution functions

#define PORT 8080

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Hello, Server Started\n");

    // Initialize scheduler
    init_scheduler();

    int client_number = 1;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd;

        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            perror("accept");
            continue;
        }

        // Create client info structure
        client_info* info = malloc(sizeof(client_info));
        if (!info) {
            perror("malloc failed");
            close(client_fd);
            continue;
        }

        info->client_fd = client_fd;
        info->client_addr = client_addr;
        info->client_number = client_number++;
        
        // Log client connection with the required format
        printf("[%d]<<< client connected\n", info->client_number);

        // Create thread to handle client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)info) != 0) {
            perror("pthread_create failed");
            close(info->client_fd);
            free(info);
        } else {
            pthread_detach(thread_id);
        }
    }

    close(server_fd);
    return 0;
}
