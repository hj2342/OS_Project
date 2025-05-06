#include "task.h"
#include <stdio.h>
#include <string.h>

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

// Update task metrics after execution
void update_task_metrics(Task* task, int executed_time) {
    if (!task) return;
    task->total_executed += executed_time;
    task->remaining_time -= executed_time;
    task->execution_rounds++;
}
