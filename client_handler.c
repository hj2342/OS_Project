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
    char buffer[BUFFER_SIZE];
    ssize_t valread;

    while ((valread = read(info->client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[valread] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';

        // Check if this is a program execution command
        if (strncmp(buffer, "./", 2) == 0) {
            // Handle program execution
            Task new_task = {
                .type = TASK_PROGRAM,
                .client_fd = info->client_fd,
                .client_number = info->client_number,
                .client_addr = info->client_addr,
                .remaining_time = 0,
                .total_executed = 0,
                .execution_rounds = 0,
                .arrival_time = time(NULL),
                .output = NULL,
                .output_size = 0,
                .max_output_size = 0,
                .last_quantum = 0,
                .preempted = 0
            };
            strncpy(new_task.command, buffer, BUFFER_SIZE - 1);
            new_task.command[BUFFER_SIZE - 1] = '\0';

            if (sscanf(buffer, "./demo %d", &new_task.remaining_time) == 1) {
                pthread_mutex_lock(&queue_mutex);
                enqueue(task_queue, new_task);
                pthread_cond_signal(&queue_not_empty);
                pthread_mutex_unlock(&queue_mutex);
            }
        } else {
            // Handle shell command
            Task new_task = {
                .type = TASK_SHELL,
                .client_fd = info->client_fd,
                .client_number = info->client_number,
                .client_addr = info->client_addr,
                .remaining_time = -1,
                .total_executed = 0,
                .execution_rounds = 0,
                .arrival_time = time(NULL),
                .output = NULL,
                .output_size = 0,
                .max_output_size = 0,
                .last_quantum = 0,
                .preempted = 0
            };
            strncpy(new_task.command, buffer, BUFFER_SIZE - 1);
            new_task.command[BUFFER_SIZE - 1] = '\0';

            pthread_mutex_lock(&queue_mutex);
            enqueue(task_queue, new_task);
            pthread_cond_signal(&queue_not_empty);
            pthread_mutex_unlock(&queue_mutex);
        }
    }

    close(info->client_fd);
    free(info);
    return NULL;
}
