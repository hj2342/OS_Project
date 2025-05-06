// scheduler.h
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"           // For Task structure and TaskType
#include "queue.h"          // For Queue operations
#include "task_executor.h"  // For task execution functions
#include "client_handler.h" // For client_info definition

// External declarations for scheduler components
extern Queue* task_queue;
extern pthread_mutex_t queue_mutex;
extern pthread_mutex_t shell_mutex;
extern pthread_cond_t queue_not_empty;

// Scheduler functions
void init_scheduler();
void add_task(TaskType type, client_info* info);
void* scheduler_thread_func(void* arg);
Task* find_shortest_task(Queue* q, Task* last_executed);

#endif /* SCHEDULER_H */
