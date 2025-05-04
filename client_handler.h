// client_handler.h
#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <sys/socket.h>    // Add network headers
#include <netinet/in.h>    // For sockaddr_in

#define BUFFER_SIZE 4096   // Move constant here

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    int client_number;
} client_info;

void *handle_client(void *arg);
#endif