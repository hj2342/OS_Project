// scheduler.c
#include "scheduler.h"
#include "client_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

Queue *task_queue;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Queue implementation
Queue* create_queue() {
    Queue* q = malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->lock, NULL);
    return q;
}

void enqueue(Queue* q, Task task) {
    pthread_mutex_lock(&q->lock);
    Node* newNode = malloc(sizeof(Node));
    newNode->task = task;
    newNode->next = NULL;

    if (q->rear) {
        q->rear->next = newNode;
        q->rear = newNode;
    } else {
        q->front = q->rear = newNode;
    }
    pthread_mutex_unlock(&q->lock);
}

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

int is_empty(Queue* q) {
    pthread_mutex_lock(&q->lock);
    int empty = (q->front == NULL);
    pthread_mutex_unlock(&q->lock);
    return empty;
}

// Scheduler implementation
void init_scheduler() {
    task_queue = create_queue();
}

void add_task(TaskType type, client_info *info) {
    Task new_task = {
        .type = type,
        .client_fd = info->client_fd,
        .client_number = info->client_number,
        .client_addr = info->client_addr,
        .remaining_time = 0
    };

    // Extract duration from client buffer if it's a program task
    if (type == TASK_PROGRAM) {
        char buffer[BUFFER_SIZE];
        ssize_t valread = read(info->client_fd, buffer, BUFFER_SIZE - 1);
        if (valread > 0) {
            buffer[valread] = '\0';
            if (sscanf(buffer, "./demo %d", &new_task.remaining_time) != 1) {
                new_task.remaining_time = 0;
            }
        }
    }

    if (new_task.remaining_time > 0 || type == TASK_SHELL) {
        pthread_mutex_lock(&queue_mutex);
        enqueue(task_queue, new_task);
        pthread_mutex_unlock(&queue_mutex);
    }
}

void* scheduler_loop(void* arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        if (!is_empty(task_queue)) {
            Task task = dequeue(task_queue);
            pthread_mutex_unlock(&queue_mutex);

            if (task.type == TASK_PROGRAM) {
                // Hybrid RR + SJRF scheduling
                int quantum = (task.remaining_time <= 3) ? 3 : 7;
                int executed = (task.remaining_time < quantum) ? 
                             task.remaining_time : quantum;

                // Simulate execution with progress updates
                for (int i = 0; i < executed; i++) {
                    char progress[50];
                    snprintf(progress, sizeof(progress), 
                            "Processing %d/%d\n", i+1, task.remaining_time);
                    write(task.client_fd, progress, strlen(progress));
                    sleep(1);
                }
                
                task.remaining_time -= executed;
                
                if (task.remaining_time > 0) {
                    pthread_mutex_lock(&queue_mutex);
                    enqueue(task_queue, task);
                    pthread_mutex_unlock(&queue_mutex);
                } else {
                    close(task.client_fd);
                }
            } else {
                // Handle shell commands immediately
                client_info* info = malloc(sizeof(client_info));
                *info = (client_info){
                    .client_fd = task.client_fd,
                    .client_addr = task.client_addr,
                    .client_number = task.client_number
                };
                handle_client(info);
            }
        } else {
            pthread_mutex_unlock(&queue_mutex);
            usleep(100000); // Prevent busy waiting
        }
    }
    return NULL;
}
