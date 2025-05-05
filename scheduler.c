#include "scheduler.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>

// Global variables for scheduler
static Request* last_executed = NULL;

// Helper function to send output to client
static int send_output_to_client(Request* request) {
    fprintf(stderr, "[DEBUG] Attempting to send output to client\n");
    if (!request || !request->output || request->output_size == 0) {
        fprintf(stderr, "[DEBUG] No output to send\n");
        return 0;
    }
    fprintf(stderr, "[DEBUG] Sending %zu bytes to client\n", request->output_size);
    
    ssize_t sent = send(request->client_fd, request->output, request->output_size, 0);
    if (sent < 0) {
        fprintf(stderr, "Error sending output to client: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void* scheduler_thread_func(void* arg) {
    RequestQueue* queue = (RequestQueue*)arg;
    if (!queue) {
        fprintf(stderr, "Error: Invalid queue pointer in scheduler thread\n");
        return NULL;
    }

    while (1) {
        fprintf(stderr, "[DEBUG] Scheduler waiting for requests...\n");
        pthread_mutex_lock(&queue->mutex);

        // Wait for queue to be non-empty
        while (queue->size == 0) {
            fprintf(stderr, "[DEBUG] Queue is empty, waiting for signal...\n");
            int wait_result = pthread_cond_wait(&queue->not_empty, &queue->mutex);
            if (wait_result != 0) {
                fprintf(stderr, "[ERROR] Condition wait failed: %s\n", strerror(wait_result));
                pthread_mutex_unlock(&queue->mutex);
                continue;
            }
            fprintf(stderr, "[DEBUG] Scheduler woke up, queue size: %d\n", queue->size);
        }

        // Select next request to execute
        fprintf(stderr, "[DEBUG] Selecting next request to execute\n");
        Request* request = schedule_next_process(queue);
        if (!request) {
            fprintf(stderr, "[DEBUG] No request selected\n");
            pthread_mutex_unlock(&queue->mutex);
            continue;
        }
        fprintf(stderr, "[DEBUG] Selected request: '%s'\n", request->command);

        // Update execution rounds
        request->execution_rounds++;
        pthread_mutex_unlock(&queue->mutex);

        // Release mutex before executing
        // pthread_mutex_unlock(&queue->mutex);

        // Execute the request
        fprintf(stderr, "[DEBUG] Scheduler executing request: '%s'\n", request->command);
        int result = execute_process(request);

        // Handle execution result
        if (result != 0) {
            fprintf(stderr, "[ERROR] Error executing process: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "[DEBUG] Request execution completed successfully\n");
        }

        // Update last executed request
        last_executed = request;

        // If process is not complete, requeue it
        if (request->remaining_time > 0) {
            pthread_mutex_lock(&queue->mutex);
            enqueue(queue, request);
        } else {
            // Send output and completion signal to client
            if (request->output_size > 0) {
                send_output_to_client(request);
            }
            // Send completion signal (newline) to indicate command is done
            const char* completion = "\n";
            send(request->client_fd, completion, strlen(completion), 0);
            destroy_request(request);
        }
    }

    return NULL;
}

Request* schedule_next_process(RequestQueue* queue) {
    if (!queue || is_empty(queue)) {
        return NULL;
    }

    Request* selected = NULL;
    Request* current = queue->front;
    Request* shell_command = NULL;
    Request* shortest_job = NULL;
    int shortest_time = -1;

    // First pass: look for shell commands and find shortest remaining time
    while (current) {
        // Priority 1: Shell commands
        if (current->is_shell_command) {
            shell_command = current;
            break;
        }

        // Priority 2: Shortest remaining time (excluding last executed)
        if (current != last_executed && current->remaining_time > 0) {
            if (shortest_time == -1 || current->remaining_time < shortest_time) {
                shortest_time = current->remaining_time;
                shortest_job = current;
            }
        }

        current = current->next;
    }

    // Select based on priorities
    if (shell_command) {
        selected = shell_command;
    } else if (shortest_job) {
        selected = shortest_job;
    } else if (queue->front != last_executed) {
        // If no other option, take the first request that's not last_executed
        selected = queue->front;
    } else if (queue->front->next) {
        // If first is last_executed, take the next one
        selected = queue->front->next;
    } else {
        // If only one request exists, take it
        selected = queue->front;
    }

    // Remove selected request from queue
    if (selected) {
        // If selected is at front
        if (selected == queue->front) {
            queue->front = selected->next;
            if (!queue->front) {
                queue->rear = NULL;
            }
        } else {
            // Find and remove from middle/end
            current = queue->front;
            while (current && current->next != selected) {
                current = current->next;
            }
            if (current) {
                current->next = selected->next;
                if (!current->next) {
                    queue->rear = current;
                }
            }
        }
        selected->next = NULL;
        queue->size--;
    }

    return selected;
}

int execute_process(Request* request) {
    fprintf(stderr, "[DEBUG] execute_process called for command: '%s'\n", request->command);
    if (!request) {
        fprintf(stderr, "[ERROR] execute_process: null request\n");
        return -1;
    }

    // For shell commands
    if (request->is_shell_command) {
        fprintf(stderr, "[DEBUG] Processing shell command\n");
        // Allocate initial output buffer if needed
        if (!request->output) {
            request->max_output_size = 1024;
            request->output = malloc(request->max_output_size);
            if (!request->output) {
                return -1;
            }
            request->output_size = 0;
        }

        // Execute shell command
        fprintf(stderr, "[DEBUG] Calling execute_command for: '%s'\n", request->command);
        int result = execute_command(request->command, request->output, &request->output_size, request->max_output_size);
        request->remaining_time = 0;  // Shell commands complete in one go
        fprintf(stderr, "[DEBUG] execute_command returned: %d\n", result);
        return result;
    }

    // For program simulation
    int quantum = (request->execution_rounds == 0) ? FIRST_ROUND_QUANTUM : SUBSEQUENT_ROUND_QUANTUM;
    int time_to_execute = (request->remaining_time < quantum) ? request->remaining_time : quantum;

    // Simulate execution
    for (int i = 0; i < time_to_execute && request->remaining_time > 0; i++) {
        // Simulate one time unit of work
        sleep(1);
        request->remaining_time--;

        // Generate some output
        char temp[64];
        snprintf(temp, sizeof(temp), "Executed 1 time unit. Remaining time: %d\n", request->remaining_time);

        // Ensure output buffer has enough space
        size_t needed_size = request->output_size + strlen(temp) + 1;
        if (needed_size > request->max_output_size) {
            size_t new_size = request->max_output_size * 2;
            if (new_size < needed_size) new_size = needed_size;
            
            char* new_buffer = realloc(request->output, new_size);
            if (!new_buffer) {
                return -1;
            }
            request->output = new_buffer;
            request->max_output_size = new_size;
        }

        // Append output
        strcpy(request->output + request->output_size, temp);
        request->output_size += strlen(temp);
    }

    return 0;
}

int init_scheduler(RequestQueue* queue) {
    pthread_t scheduler_thread;
    int result = pthread_create(&scheduler_thread, NULL, scheduler_thread_func, queue);
    if (result != 0) {
        fprintf(stderr, "Error creating scheduler thread: %s\n", strerror(result));
        return -1;
    }
    
    // Detach the thread so it can run independently
    pthread_detach(scheduler_thread);
    return 0;
}
