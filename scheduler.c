// scheduler.c
#include "scheduler.h"
#include "gantt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// Global variables for scheduler
Queue* task_queue = NULL;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shell_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;
static pthread_t scheduler_thread;

// Find the shortest task in the queue, excluding the last executed task
Task* find_shortest_task(Queue* q, Task* last_executed) {
    if (!q || !q->front) return NULL;

    Node* current = q->front;
    Node* shortest = NULL;
    
    // First pass: find shortest task, including the last executed one
    while (current) {
        if (current->task.type == TASK_PROGRAM) {
            if (!shortest || 
                current->task.remaining_time < shortest->task.remaining_time) {
                shortest = current;
            }
        }
        current = current->next;
    }
    
    // If we found no program tasks
    if (!shortest) return NULL;
    
    // If this is the same as last executed task and it's not the only task
    if (last_executed && 
        shortest->task.client_fd == last_executed->client_fd && 
        get_queue_size(q) > 1) {
        
        // Try to find next best task
        current = q->front;
        Node* next_best = NULL;
        
        while (current) {
            if (current->task.type == TASK_PROGRAM && 
                current->task.client_fd != last_executed->client_fd) {
                if (!next_best || 
                    current->task.remaining_time < next_best->task.remaining_time) {
                    next_best = current;
                }
            }
            current = current->next;
        }
        
        // If we found an alternative and it's not much longer
        if (next_best && 
            next_best->task.remaining_time - shortest->task.remaining_time <= PREEMPTION_THRESHOLD) {
            shortest = next_best;
        }
    }
    
    return &(shortest->task);
}

void* scheduler_thread_func(void* arg) {
    (void)arg;  // Suppress unused parameter warning
    printf("[SCHEDULER] Thread started\n");
    int last_executed_client = -1; // Store client number instead of task pointer
    int current_round = 0;

    while (1) {
        printf("[SCHEDULER] Round %d: Checking queue...\n", current_round);

        pthread_mutex_lock(&queue_mutex);

        while (is_empty(task_queue)) {
            printf("[SCHEDULER] Queue empty, waiting for tasks...\n");
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

        // Get a copy of the task to execute
        Task task_to_execute;
        Node* selected_node = NULL;
        
        // Check if front task is a shell command
        Task* front_task = peek_queue(task_queue);
        if (front_task && front_task->type == TASK_SHELL) {
            // For shell tasks, always execute them immediately
            task_to_execute = dequeue(task_queue);
        } else {
            // For program tasks, apply SJF with preemption
            Task* shortest = find_shortest_task(task_queue, NULL);
            
            if (should_preempt(front_task, shortest)) {
                printf("[SCHEDULER] Preempting current task for shorter task\n");
                // Get a copy of the shortest task
                selected_node = NULL;
                for (Node* temp = task_queue->front; temp != NULL; temp = temp->next) {
                    if (&(temp->task) == shortest) {
                        selected_node = temp;
                        break;
                    }
                }
                
                if (selected_node) {
                    task_to_execute = selected_node->task;
                    task_to_execute.preempted = 1;
                    remove_task(task_queue, shortest);
                } else {
                    // Fallback to front task if shortest not found
                    task_to_execute = dequeue(task_queue);
                }
            } else {
                // No preemption, take the front task
                task_to_execute = dequeue(task_queue);
            }
        }
        
        pthread_mutex_unlock(&queue_mutex);

        // Record execution start time for Gantt chart
        int start_time = current_round;
        int task_id = task_to_execute.client_number * 100 + task_to_execute.execution_rounds;
        
        // Execute the task (using our local copy)
        if (task_to_execute.type == TASK_SHELL) {
            execute_shell_task(&task_to_execute);
            
            // Add shell command execution to Gantt chart (duration 1)
            add_execution_record(task_to_execute.client_number, task_id, start_time, 1);
        } else {
            int quantum = calculate_quantum(&task_to_execute);
            execute_program_task(&task_to_execute, quantum);
            
            // Calculate actual execution duration (quantum or less if completed)
            int duration = quantum;
            if (task_to_execute.remaining_time == 0) {
                // Task completed, might have used less than the full quantum
                duration = quantum - task_to_execute.last_quantum;
            }
            
            // Add program execution to Gantt chart
            add_execution_record(task_to_execute.client_number, task_id, start_time, duration);
            
            // Re-enqueue if task not complete
            if (task_to_execute.remaining_time > 0) {
                printf("[SCHEDULER] Re-enqueueing task with %d time remaining\n", 
                       task_to_execute.remaining_time);
                pthread_mutex_lock(&queue_mutex);
                enqueue(task_queue, task_to_execute);
                pthread_mutex_unlock(&queue_mutex);
            }
        }

        // Update scheduling state with client number
        last_executed_client = task_to_execute.client_number;
        current_round++;

        printf("[SCHEDULER] Round %d completed\n", current_round);
        
        // Check if we should display the Gantt chart
        // Only display when queue is empty and we just completed a task
        pthread_mutex_lock(&queue_mutex);
        int queue_empty = is_empty(task_queue);
        pthread_mutex_unlock(&queue_mutex);
        
        if (queue_empty && task_to_execute.type == TASK_PROGRAM && task_to_execute.remaining_time == 0) {
            // We just completed the last program task and queue is empty
            // Display the Gantt chart
            print_gantt_chart();
        }
    }

    return NULL;
}

void init_scheduler() {
    task_queue = create_queue();
    if (!task_queue) {
        fprintf(stderr, "Failed to create task queue\n");
        exit(1);
    }
    
    // Initialize the execution history for Gantt chart
    init_execution_history();
    
    if (pthread_create(&scheduler_thread, NULL, scheduler_thread_func, NULL) != 0) {
        fprintf(stderr, "Failed to create scheduler thread\n");
        exit(1);
    }
}

void add_task(TaskType type, client_info *info) {
    Task new_task = {
        .type = type,
        .client_fd = info->client_fd,
        .client_number = info->client_number,
        .client_addr = info->client_addr,
        .remaining_time = (type == TASK_SHELL) ? -1 : 0,
        .execution_rounds = 0,
        .arrival_time = time(NULL),
        .preempted = 0,
        .total_executed = 0,
        .initial_n = 0  // Initialize to 0
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
            // Store the initial N value when we first get it
            new_task.initial_n = new_task.remaining_time;
        }
    }

    if (new_task.remaining_time > 0 || type == TASK_SHELL) {
        pthread_mutex_lock(&queue_mutex);
        enqueue(task_queue, new_task);
        pthread_mutex_unlock(&queue_mutex);
    }
}

