#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

RequestQueue* create_queue(void) {
    RequestQueue* queue = (RequestQueue*)malloc(sizeof(RequestQueue));
    if (!queue) {
        return NULL;
    }

    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue);
        return NULL;
    }
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue);
        return NULL;
    }

    return queue;
}

void destroy_queue(RequestQueue* queue) {
    if (!queue) return;

    // Free all requests in the queue
    while (!is_empty(queue)) {
        Request* request = dequeue(queue);
        destroy_request(request);
    }

    // Destroy synchronization primitives
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    free(queue);
}

Request* create_request(int client_fd, const char* command, bool is_shell_command, int burst_time) {
    Request* request = (Request*)malloc(sizeof(Request));
    if (!request) {
        return NULL;
    }

    // Initialize all fields
    request->client_fd = client_fd;
    request->command = strdup(command);
    if (!request->command) {
        free(request);
        return NULL;
    }

    request->is_shell_command = is_shell_command;
    request->burst_time = burst_time;
    request->remaining_time = burst_time;
    request->arrival_time = time(NULL);
    request->execution_rounds = 0;
    request->output = NULL;
    request->output_size = 0;
    request->max_output_size = 0;
    request->next = NULL;

    return request;
}

void destroy_request(Request* request) {
    if (!request) return;

    free(request->command);
    free(request->output);
    free(request);
}

bool enqueue(RequestQueue* queue, Request* request) {
    fprintf(stderr, "[DEBUG] Enqueue called with request: '%s'\n", request ? request->command : "NULL");
    if (!queue || !request) {
        fprintf(stderr, "[ERROR] Enqueue failed: %s\n", !queue ? "NULL queue" : "NULL request");
        return false;
    }

    pthread_mutex_lock(&queue->mutex);

    // Add the request to the queue
    fprintf(stderr, "[DEBUG] Adding request to queue (size before: %d)\n", queue->size);
    if (queue->size == 0) {
        queue->front = request;
        queue->rear = request;
    } else {
        queue->rear->next = request;
        queue->rear = request;
    }
    queue->size++;
    fprintf(stderr, "[DEBUG] Request added, new size: %d\n", queue->size);

    // Signal that queue is not empty
    fprintf(stderr, "[DEBUG] Signaling scheduler thread\n");
    int signal_result = pthread_cond_signal(&queue->not_empty);
    if (signal_result != 0) {
        fprintf(stderr, "[ERROR] Failed to signal scheduler: %s\n", strerror(signal_result));
    }
    fprintf(stderr, "[DEBUG] Releasing queue mutex\n");
    pthread_mutex_unlock(&queue->mutex);

    return true;
}

Request* dequeue(RequestQueue* queue) {
    if (!queue || is_empty(queue)) {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);

    // Remove the request from the front
    Request* request = queue->front;
    queue->front = request->next;
    request->next = NULL;
    queue->size--;

    if (queue->size == 0) {
        queue->rear = NULL;
    }

    pthread_mutex_unlock(&queue->mutex);
    return request;
}

Request* peek(RequestQueue* queue) {
    if (!queue || is_empty(queue)) {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);
    Request* request = queue->front;
    pthread_mutex_unlock(&queue->mutex);

    return request;
}

bool is_empty(RequestQueue* queue) {
    if (!queue) return true;
    return (queue->size == 0);
}
