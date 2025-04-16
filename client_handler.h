// // client_handler.h
// #ifndef CLIENT_HANDLER_H
// #define CLIENT_HANDLER_H

// void *handle_client(void *arg);

// #endif
// client_handler.h
#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <netinet/in.h>

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    int client_number;
} client_info;

void *handle_client(void *arg);

#endif
