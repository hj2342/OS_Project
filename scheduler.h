#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"

// Constants for scheduler
#define FIRST_ROUND_QUANTUM 3
#define SUBSEQUENT_ROUND_QUANTUM 7

// Scheduler thread function
void* scheduler_thread_func(void* arg);

// Schedule next process based on RR and SRJF
Request* schedule_next_process(RequestQueue* queue);

// Execute a request
int execute_process(Request* request);

// Initialize scheduler
int init_scheduler(RequestQueue* queue);

#endif // SCHEDULER_H
