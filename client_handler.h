// client_handler.h
#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "task.h"         // Include task.h for Task structure and BUFFER_SIZE
#include <sys/socket.h>    // Add network headers
#include <netinet/in.h>    // For sockaddr_in

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    int client_number;
} client_info;

void *handle_client(void *arg);
#endif /* CLIENT_HANDLER_H */