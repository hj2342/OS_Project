#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

// Create a new queue
Queue* create_queue() {
    Queue* q = malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return q;
}

// Add a task to the queue
void enqueue(Queue* q, Task task) {
    pthread_mutex_lock(&q->lock);
    Node* newNode = malloc(sizeof(Node));
    if (!newNode) {
        pthread_mutex_unlock(&q->lock);
        return;
    }
    newNode->task = task;
    newNode->next = NULL;

    if (q->rear) {
        q->rear->next = newNode;
        q->rear = newNode;
    } else {
        q->front = q->rear = newNode;
    }
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

// Remove and return the first task from the queue
Task dequeue(Queue* q) {
    pthread_mutex_lock(&q->lock);
    if (!q->front) {
        pthread_mutex_unlock(&q->lock);
        return (Task){0};
    }

    Node* temp = q->front;
    Task task = temp->task;
    q->front = q->front->next;
    
    if (!q->front) q->rear = NULL;
    free(temp);
    pthread_mutex_unlock(&q->lock);
    return task;
}

// Check if the queue is empty
int is_empty(Queue* q) {
    pthread_mutex_lock(&q->lock);
    int empty = (q->front == NULL);
    pthread_mutex_unlock(&q->lock);
    return empty;
}

// Return a pointer to the first task without removing it
Task* peek_queue(Queue* q) {
    if (!q || !q->front) return NULL;
    return &(q->front->task);
}

// Get the number of tasks in the queue
int get_queue_size(Queue* q) {
    if (!q) return 0;
    int size = 0;
    Node* current = q->front;
    while (current) {
        size++;
        current = current->next;
    }
    return size;
}

// Remove a specific task from the queue
void remove_task(Queue* q, Task* task) {
    if (!q || !q->front || !task) return;
    
    Node* current = q->front;
    Node* prev = NULL;
    
    while (current) {
        if (&(current->task) == task) {
            if (prev) {
                prev->next = current->next;
            } else {
                q->front = current->next;
            }
            if (current == q->rear) {
                q->rear = prev;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}
