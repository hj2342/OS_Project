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
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN);
    int port = ntohs(client_addr.sin_port);
    
    free(info);
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

        // Check for program command
        if (strncmp(buffer, "./demo ", 7) == 0) {
            int n = atoi(buffer + 7);
            if (n > 0) {
                Task new_task = {
                    .type = TASK_PROGRAM,
                    .client_fd = client_fd,
                    .remaining_time = n,
                    .client_addr = client_addr
                };

                pthread_mutex_lock(&queue_mutex);
                enqueue(task_queue, new_task);
                pthread_mutex_unlock(&queue_mutex);

                const char *msg = "Program added to scheduler.\n";
                write(client_fd, msg, strlen(msg));
                printf("[SCHEDULED] [Client #%d - %s:%d] Added demo(%d)\n", 
                      client_number, ip, port, n);
                continue;
            }
        }

        // Existing command handling logic
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

        if (strcmp(buffer, "exit") == 0) {
            printf("[INFO] [Client #%d - %s:%d] Exit command received\n", 
                  client_number, ip, port);
            break;
        }

        // Execute shell command immediately
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            perror("[ERROR] Pipe failed");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) { // Child process
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            execlp("sh", "sh", "-c", buffer, NULL);
            perror("execlp");
            exit(EXIT_FAILURE);
        } else { // Parent process
            close(pipefd[1]);
            char output[BUFFER_SIZE] = {0};
            ssize_t bytes_read;
            
            while ((bytes_read = read(pipefd[0], output, BUFFER_SIZE - 1)) > 0) {
                write(client_fd, output, bytes_read);
                memset(output, 0, BUFFER_SIZE);
            }
            
            close(pipefd[0]);
            waitpid(pid, NULL, 0);
        }
    }

    close(client_fd);
    return NULL;
}
