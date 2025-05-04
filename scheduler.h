// scheduler.h
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <sys/socket.h>    // Add missing network headers
#include <netinet/in.h>    // For sockaddr_in
#include "client_handler.h" // For client_info definition

#define BUFFER_SIZE 4096   // Add missing constant

typedef enum { TASK_SHELL, TASK_PROGRAM } TaskType;

typedef struct {
    TaskType type;
    int client_fd;
    int remaining_time;
    struct sockaddr_in client_addr; // Now properly defined
    int client_number;
} Task;

typedef struct Node {
    Task task;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* rear;
    pthread_mutex_t lock;
} Queue;

// Queue operations
Queue* create_queue();
void enqueue(Queue* q, Task task);
Task dequeue(Queue* q);
int is_empty(Queue* q);

// Scheduler functions
void init_scheduler();
void add_task(TaskType type, client_info* info);
void* scheduler_loop(void* arg);
void execute_program_task(Task task);

#endif
