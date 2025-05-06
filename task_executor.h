#ifndef TASK_EXECUTOR_H
#define TASK_EXECUTOR_H

#include "task.h"

// Constants for execution
#define FIRST_QUANTUM 3  // 3 seconds for the first round
#define OTHER_QUANTUM 7  // 7 seconds for all subsequent rounds
#define PREEMPTION_THRESHOLD 5

// Execution functions
void execute_program_task(Task* task, int quantum);
void execute_shell_task(Task* task);
int calculate_quantum(const Task* task);
int should_preempt(const Task* current, const Task* shortest);

#endif /* TASK_EXECUTOR_H */
