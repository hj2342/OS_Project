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
    struct sockaddr_in client_addr;
    int client_number;
    char command[BUFFER_SIZE];
    int execution_rounds;     // Number of times this task has been scheduled
    time_t arrival_time;     // Time when task was first added
    char* output;           // Buffer for storing task output
    size_t output_size;     // Current size of output buffer
    size_t max_output_size; // Maximum allocated size of output buffer
    int last_quantum;       // Last quantum size used for this task
    int preempted;          // Whether this task was preempted
    int total_executed;     // Total time executed so far
} Task;

typedef struct Node {
    Task task;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* rear;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} Queue;

// External declarations
extern Queue* task_queue;
extern pthread_mutex_t queue_mutex;
extern pthread_cond_t queue_not_empty;

// Queue operations
Queue* create_queue();
void enqueue(Queue* q, Task task);
Task dequeue(Queue* q);
int is_empty(Queue* q);
Task* peek_queue(Queue* q);           // Look at next task without removing
Task* find_shortest_task(Queue* q, Task* last_executed);   // Find task with shortest remaining time
int get_queue_size(Queue* q);         // Get current queue size
void remove_task(Queue* q, Task* task); // Remove specific task from queue

// Scheduling constants
#define FIRST_QUANTUM 3
#define OTHER_QUANTUM 7
#define PREEMPTION_THRESHOLD 2  // Preempt if difference > threshold

// Task execution functions
void execute_program_task(Task* task, int quantum);
void execute_shell_task(Task* task);

// Scheduling helper functions
int calculate_quantum(Task* task);
int should_preempt(Task* current, Task* shortest);
void update_task_metrics(Task* task, int executed_time);

// Scheduler functions
void init_scheduler();
void add_task(TaskType type, client_info* info);
void* scheduler_loop(void* arg);

#endif
