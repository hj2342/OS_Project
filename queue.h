#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include "task.h"

// Queue structure definitions
typedef struct Node {
    Task task;
    struct Node* next;
} Node;

typedef struct Queue {
    Node* front;
    Node* rear;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} Queue;

// Queue operations
Queue* create_queue();
void enqueue(Queue* q, Task task);
Task dequeue(Queue* q);
int is_empty(Queue* q);
Task* peek_queue(Queue* q);
int get_queue_size(Queue* q);
void remove_task(Queue* q, Task* task);

#endif /* QUEUE_H */
