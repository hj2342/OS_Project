#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <netinet/in.h>
#include "queue.h"

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    int client_number;
    RequestQueue* request_queue;  // Added request queue pointer
} client_info;

// Initialize the client handler with a request queue
void init_client_handler(RequestQueue* queue);

// Handle client connection
void* handle_client(void* arg);

#endif
