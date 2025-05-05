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
#include <limits.h>
#include <time.h>

Queue* task_queue = NULL;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;
static pthread_t scheduler_thread;

// Function declarations
void execute_program_task(Task* task, int quantum);
void execute_shell_task(Task* task);
void* scheduler_thread_func(void* arg);
void init_scheduler(void);
void add_task(TaskType type, client_info* info);
void* scheduler_loop(void* arg);

// Queue implementation
Queue* create_queue() {
    Queue* q = malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return q;
}

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
static pthread_t scheduler_thread;

// Initialize a task's output buffer
void init_task_output(Task* task) {
    task->output = NULL;
    task->output_size = 0;
    task->max_output_size = 0;
}

// Add output to task's buffer, reallocating if necessary
void append_task_output(Task* task, const char* new_output, size_t length) {
    size_t new_size = task->output_size + length + 1;  // +1 for null terminator
    
    if (new_size > task->max_output_size) {
        size_t new_max = (new_size * 3) / 2;  // Grow by 1.5x
        char* new_buf = realloc(task->output, new_max);
        if (!new_buf) {
            perror("[ERROR] Failed to reallocate output buffer");
            return;
        }
        task->output = new_buf;
        task->max_output_size = new_max;
    }
    
    memcpy(task->output + task->output_size, new_output, length);
    task->output_size += length;
    task->output[task->output_size] = '\0';
}


Task* peek_queue(Queue* q) {
    if (!q || !q->front) return NULL;
    return &(q->front->task);
}

// Find the shortest task in the queue, excluding the last executed task
Task* find_shortest_task(Queue* q, Task* last_executed) {
    if (!q || !q->front) return NULL;

    Node* current = q->front;
    Node* shortest = NULL;
    
    // First pass: find shortest non-shell task that isn't the last executed
    while (current) {
        // Skip shell commands and last executed task unless it's the only option
        if (current->task.type == TASK_PROGRAM && 
            (!last_executed || 
             current->task.client_fd != last_executed->client_fd || 
             get_queue_size(q) == 1)) {
            if (!shortest || 
                current->task.remaining_time < shortest->task.remaining_time) {
                shortest = current;
            }
        }
        current = current->next;
    }
    
    return &(shortest->task);
}

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

int calculate_quantum(Task* task) {
    if (!task) return 0;
    return (task->execution_rounds == 0) ? FIRST_QUANTUM : OTHER_QUANTUM;
}

int should_preempt(Task* current, Task* shortest) {
    if (!current || !shortest) return 0;
    if (current == shortest) return 0;
    
    // Only preempt program tasks
    if (current->type != TASK_PROGRAM || shortest->type != TASK_PROGRAM) {
        return 0;
    }
    
    return (current->remaining_time - shortest->remaining_time) > PREEMPTION_THRESHOLD;
}

void update_task_metrics(Task* task, int executed_time) {
    if (!task) return;
    task->total_executed += executed_time;
    task->remaining_time -= executed_time;
    task->execution_rounds++;
}

void execute_program_task(Task* task, int quantum) {
    printf("\033[34m[%d]--- created (%d)\033[0m\n", 
           task->client_number, task->remaining_time);

    printf("\033[32m[%d]--- started (%d)\033[0m\n", 
           task->client_number, task->remaining_time);

    int to_execute = (task->remaining_time < quantum) ? 
                    task->remaining_time : quantum;
    
    for (int i = 0; i < to_execute && task->remaining_time > 0; i++) {
        printf("\033[33m[%d]--- running (%d)\033[0m\n", 
               task->client_number, task->remaining_time);

        // Send progress to client
        char progress[50];
        snprintf(progress, sizeof(progress), "Demo %d/%d\n", 
                 task->total_executed + i + 1, 
                 task->remaining_time + task->total_executed);
        write(task->client_fd, progress, strlen(progress));
        
        sleep(1);
        task->remaining_time--;
    }

    task->total_executed += to_execute;
    task->execution_rounds++;

    if (task->remaining_time > 0) {
        printf("\033[35m[%d]--- waiting (%d)\033[0m\n", 
               task->client_number, task->remaining_time);
    } else {
        printf("\033[31m[%d]--- ended (0)\033[0m\n", 
               task->client_number);
    }
}

void execute_shell_task(Task* task) {
    printf("%s[%d]--- created (-1)%s\n", 
           COLOR_CYAN, task->client_number, COLOR_RESET);
    printf("%s[%d]--- started (-1)%s\n", 
           COLOR_GREEN, task->client_number, COLOR_RESET);
    
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid == 0) {  // Child process
        close(pipefd[0]);  // Close read end
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        // Execute command
        char* args[] = {"/bin/sh", "-c", task->command, NULL};
        execv("/bin/sh", args);
        perror("execv");
        exit(1);
    } else {  // Parent process
        close(pipefd[1]);  // Close write end
        
        char buffer[1024];
        ssize_t bytes_read;
        size_t total_bytes = 0;
        
        // Read and send output to client
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            write(task->client_fd, buffer, bytes_read);
            total_bytes += bytes_read;
        }
        
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        
        // Send bytes sent message
        char msg[50];
        snprintf(msg, sizeof(msg), "<<< %zu bytes sent\n", total_bytes);
        write(task->client_fd, msg, strlen(msg));
        
        printf("%s[%d]--- ended (-1)%s\n", 
               COLOR_RED, task->client_number, COLOR_RESET);
    }
}

void* scheduler_thread_func(void* arg) {
    (void)arg;  // Suppress unused parameter warning
    printf("[SCHEDULER] Thread started\n");
    Task* last_executed = NULL;
    int current_round = 0;

    while (1) {
        printf("[SCHEDULER] Round %d: Checking queue...\n", current_round);

        pthread_mutex_lock(&queue_mutex);

        while (is_empty(task_queue)) {
            printf("[SCHEDULER] Queue empty, waiting for tasks...\n");
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

        // Get next task and check for preemption
        Task* current = peek_queue(task_queue);
        Task* shortest = find_shortest_task(task_queue, NULL);  // No last executed in this context
        
        // Decide which task to execute
        Task* to_execute = current;
        if (should_preempt(current, shortest)) {
            printf("[SCHEDULER] Preempting current task for shorter task\n");
            to_execute = shortest;
            remove_task(task_queue, shortest);
            to_execute->preempted = 1;
        } else {
            Task temp = dequeue(task_queue);  // Get a copy of the task
            to_execute = &temp;
            to_execute->preempted = 0;
        }

        // Don't execute the same task consecutively unless it's the only one
        if (last_executed == to_execute && get_queue_size(task_queue) > 0) {
            printf("[SCHEDULER] Skipping consecutive execution of same task\n");
            enqueue(task_queue, *to_execute);
            pthread_mutex_unlock(&queue_mutex);
            continue;
        }

        pthread_mutex_unlock(&queue_mutex);

        // Execute the task
        if (to_execute->type == TASK_SHELL) {
            execute_shell_task(to_execute);
        } else {
            int quantum = calculate_quantum(to_execute);
            execute_program_task(to_execute, quantum);
            
            // Re-enqueue if task not complete
            if (to_execute->remaining_time > 0) {
                printf("[SCHEDULER] Re-enqueueing task with %d time remaining\n", 
                       to_execute->remaining_time);
                pthread_mutex_lock(&queue_mutex);
                enqueue(task_queue, *to_execute);
                pthread_mutex_unlock(&queue_mutex);
            }
        }

        // Update scheduling state
        last_executed = to_execute;
        current_round++;

        printf("[SCHEDULER] Round %d completed\n", current_round);
    }

    return NULL;
}

void init_scheduler() {
    task_queue = create_queue();
    if (!task_queue) {
        fprintf(stderr, "Failed to create task queue\n");
        exit(1);
    }
    
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
        .total_executed = 0
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
    printf("[SCHEDULER] Thread started\n");
    int current_round = 0;
    Task* last_executed = NULL;

    while (1) {
        pthread_mutex_lock(&queue_mutex);
        if (is_empty(task_queue)) {
            pthread_cond_wait(&queue_not_empty, &queue_mutex);
        }

        Task* to_execute = peek_queue(task_queue);
        if (!to_execute) {
            pthread_mutex_unlock(&queue_mutex);
            continue;
        }

        // Handle shell commands immediately
        if (to_execute->type == TASK_SHELL) {
            remove_task(task_queue, to_execute);
            pthread_mutex_unlock(&queue_mutex);
            execute_shell_task(to_execute);
            // Print summary for shell command
            printf("%s[%d] = (%d) -> (-1) = (-1) -> (1)%s\n", 
                   BG_BLUE, current_round++, to_execute->client_number, COLOR_RESET);
            continue;
        }

        // For program tasks, check for shorter tasks
        Task* shortest = find_shortest_task(task_queue, last_executed);
        if (shortest && shortest != to_execute && 
            should_preempt(to_execute, shortest)) {
            to_execute = shortest;
        }

        // Remove the task from queue
        remove_task(task_queue, to_execute);
        pthread_mutex_unlock(&queue_mutex);

        // Execute program task
        int quantum = calculate_quantum(to_execute);
        execute_program_task(to_execute, quantum);
        
        // Re-enqueue if task not complete
        if (to_execute->remaining_time > 0) {
            pthread_mutex_lock(&queue_mutex);
            enqueue(task_queue, *to_execute);
            pthread_mutex_unlock(&queue_mutex);
        }

        // Update scheduling state and print summary
        last_executed = to_execute;
        printf("%s[%d] = (%d) -> (%d) = (%d) -> (%d)%s\n", 
               BG_BLUE, current_round++, 
               to_execute->client_number,
               to_execute->total_executed,
               to_execute->remaining_time,
               to_execute->execution_rounds,
               COLOR_RESET);
    }
    return NULL;
}
