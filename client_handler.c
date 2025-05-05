#include "client_handler.h"
#include "executor.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 4096
#define MAX_CMDS 10

// External declarations from scheduler
extern pthread_mutex_t queue_mutex;
extern Queue *task_queue;

void *handle_client(void *arg) {
    client_info *info = (client_info *)arg;
    int client_fd = info->client_fd;
    int client_number = info->client_number;
    struct sockaddr_in client_addr = info->client_addr;
    
    printf("[CLIENT #%d] Handler thread started\n", client_number);
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN);
    int port = ntohs(client_addr.sin_port);
    
    char buffer[BUFFER_SIZE] = {0};

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t valread = read(client_fd, buffer, BUFFER_SIZE - 1);
        
        if (valread <= 0) {
            printf("[INFO] [Client #%d - %s:%d] Client disconnected.\n", 
                  client_number, ip, port);
            break;
        }

        buffer[strcspn(buffer, "\r\n")] = '\0';

        // Input validation
        int all_whitespace = 1, has_control = 0;
        for (size_t i = 0; i < strlen(buffer); i++) {
            if (buffer[i] < 32 || buffer[i] == 127) has_control = 1;
            if (buffer[i] != ' ') all_whitespace = 0;
        }

        if (strlen(buffer) == 0 || all_whitespace || has_control) {
            const char *err_msg = "Error: Invalid input\n";
            write(client_fd, err_msg, strlen(err_msg));
            continue;
        }

        printf("[RECEIVED] [Client #%d - %s:%d] Command: \"%s\"\n", 
               client_number, ip, port, buffer);

        // Check if this is a program execution command
        if (strncmp(buffer, "./", 2) == 0) {
            // Handle program execution
            Task new_task = {
                .type = TASK_PROGRAM,
                .client_fd = client_fd,
                .client_addr = client_addr,
                .client_number = client_number,
                .remaining_time = 0,
                .execution_rounds = 0,
                .arrival_time = time(NULL),
                .preempted = 0,
                .total_executed = 0
            };

            if (sscanf(buffer, "./demo %d", &new_task.remaining_time) == 1) {
                printf("[CLIENT #%d] Enqueueing program task (duration: %d)\n", 
                       client_number, new_task.remaining_time);
                pthread_mutex_lock(&queue_mutex);
                enqueue(task_queue, new_task);
                pthread_cond_signal(&queue_not_empty);
                pthread_mutex_unlock(&queue_mutex);
                printf("[CLIENT #%d] Program task enqueued\n", client_number);

                printf("[SCHEDULED] [Client #%d - %s:%d] Added demo(%d)\n", 
                       client_number, ip, port, new_task.remaining_time);
            }
        } else {
            // Handle shell command
            Task new_task = {
                .type = TASK_SHELL,
                .client_fd = client_fd,
                .client_addr = client_addr,
                .client_number = client_number,
                .remaining_time = 0,
                .execution_rounds = 0,
                .arrival_time = time(NULL),
                .preempted = 0,
                .total_executed = 0
            };

            strncpy(new_task.command, buffer, BUFFER_SIZE - 1);
            new_task.command[BUFFER_SIZE - 1] = '\0';

            printf("[CLIENT #%d] Enqueueing shell task: %s\n", client_number, buffer);
            pthread_mutex_lock(&queue_mutex);
            enqueue(task_queue, new_task);
            pthread_cond_signal(&queue_not_empty);
            pthread_mutex_unlock(&queue_mutex);
            printf("[CLIENT #%d] Shell task enqueued\n", client_number);

            printf("[SCHEDULED] [Client #%d - %s:%d] Added shell command: %s\n", 
                  client_number, ip, port, buffer);
        }
    }

    close(client_fd);
    return NULL;
}
