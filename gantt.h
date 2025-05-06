#ifndef GANTT_H
#define GANTT_H

#include "task.h"

// Maximum number of execution records in global history
#define MAX_GLOBAL_RECORDS 200

// Global execution history for Gantt chart
typedef struct {
    int record_count;
    ExecutionRecord records[MAX_GLOBAL_RECORDS];
    int current_round;
} ExecutionHistory;

// Initialize the execution history
void init_execution_history();

// Reset the execution history (clear all records)
void reset_execution_history();

// Add an execution record to the global history
void add_execution_record(int client_number, int task_id, int start_time, int duration);

// Print the Gantt chart
void print_gantt_chart();

// Get the current execution history
ExecutionHistory* get_execution_history();

#endif /* GANTT_H */
