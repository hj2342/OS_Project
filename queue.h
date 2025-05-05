#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <time.h>
#include <pthread.h>

// Request structure representing a client's command request
typedef struct Request {
    int client_fd;              // Client's file descriptor
    char* command;             // Original command string
    bool is_shell_command;     // Flag (true if shell command, false if program)
    int burst_time;           // Total execution time (-1 for shell commands)
    int remaining_time;       // Time remaining for execution
    time_t arrival_time;      // Time of request arrival
    int execution_rounds;     // Number of times scheduled
    char* output;             // Buffer for command output
    size_t output_size;       // Current output size
    size_t max_output_size;   // Maximum allocated output size
    struct Request* next;     // Pointer to next request in queue
} Request;

// Queue structure
typedef struct {
    Request* front;           // Front of the queue
    Request* rear;            // Rear of the queue
    pthread_mutex_t mutex;    // Mutex for thread-safe operations
    pthread_cond_t not_empty; // Condition variable for queue not empty
    int size;                // Current size of the queue
} RequestQueue;

// Queue operations
RequestQueue* create_queue(void);
void destroy_queue(RequestQueue* queue);
bool enqueue(RequestQueue* queue, Request* request);
Request* dequeue(RequestQueue* queue);
Request* peek(RequestQueue* queue);
bool is_empty(RequestQueue* queue);

// Request operations
Request* create_request(int client_fd, const char* command, bool is_shell_command, int burst_time);
void destroy_request(Request* request);

#endif // QUEUE_H
