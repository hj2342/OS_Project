#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gantt.h"

// Global execution history
static ExecutionHistory execution_history;

// Color codes for the Gantt chart
static const char* client_colors[] = {
    "\033[0m",    // Default (not used)
    "\033[34m",   // Blue
    "\033[32m",   // Green
    "\033[33m",   // Yellow
    "\033[31m",   // Red
    "\033[35m",   // Magenta
    "\033[36m",   // Cyan
    "\033[37m",   // White
    "\033[1;34m", // Bright Blue
    "\033[1;32m", // Bright Green
    "\033[1;33m"  // Bright Yellow
};
#define NUM_COLORS (sizeof(client_colors) / sizeof(client_colors[0]))
#define COLOR_RESET "\033[0m"

// Initialize the execution history
void init_execution_history() {
    execution_history.record_count = 0;
    execution_history.current_round = 0;
    memset(execution_history.records, 0, sizeof(execution_history.records));
}

// Add an execution record to the global history
void add_execution_record(int client_number, int task_id, int start_time, int duration) {
    if (execution_history.record_count >= MAX_GLOBAL_RECORDS) {
        printf("[GANTT] Warning: Maximum execution records reached, cannot add more.\n");
        return;
    }
    
    ExecutionRecord* record = &execution_history.records[execution_history.record_count++];
    record->client_number = client_number;
    record->task_id = task_id;
    record->start_time = start_time;
    record->duration = duration;
}

// Get the current execution history
ExecutionHistory* get_execution_history() {
    return &execution_history;
}

// Increment the current round
void increment_round() {
    execution_history.current_round++;
}

// Print the Gantt chart in the format (time)-P{client_number}-(duration)
void print_gantt_chart() {
    if (execution_history.record_count == 0) {
        printf("No execution records to display.\n");
        return;
    }
    
    // Sort records by start time
    for (int i = 0; i < execution_history.record_count - 1; i++) {
        for (int j = 0; j < execution_history.record_count - i - 1; j++) {
            if (execution_history.records[j].start_time > execution_history.records[j + 1].start_time) {
                // Swap records
                ExecutionRecord temp = execution_history.records[j];
                execution_history.records[j] = execution_history.records[j + 1];
                execution_history.records[j + 1] = temp;
            }
        }
    }
    
    // Fix overlapping execution times by adjusting start times
    // to ensure sequential execution
    int current_end_time = 0;
    for (int i = 0; i < execution_history.record_count; i++) {
        ExecutionRecord* record = &execution_history.records[i];
        
        // If this record starts before the previous one ended,
        // adjust its start time
        if (record->start_time < current_end_time) {
            record->start_time = current_end_time;
        }
        
        // Update the current end time
        current_end_time = record->start_time + record->duration;
    }
    
    printf("\n\n");
    printf("===== SCHEDULER GANTT CHART =====\n\n");
    
    // Print the compact Gantt chart with colors
    printf("Gantt Chart: ");
    
    // Print the first time marker
    printf("(0)-");
    
    // Track the current time for the chart
    int current_time = 0;
    
    for (int i = 0; i < execution_history.record_count; i++) {
        ExecutionRecord* record = &execution_history.records[i];
        int client = record->client_number;
        int duration = record->duration;
        
        // Get color for this client
        const char* color = client_colors[client % NUM_COLORS];
        
        // Print the process with color
        printf("%sP%d%s-", color, client, COLOR_RESET);
        
        // Update the current time
        current_time += duration;
        
        // Print the end time
        printf("(%d)", current_time);
        
        // Add a dash if not the last record
        if (i < execution_history.record_count - 1) {
            printf("-");
        }
    }
    
    printf("\n\n");
    
    // // Print statistics
    // printf("===== SCHEDULING STATISTICS =====\n");
    
    // // Count total execution time for each client
    // int client_time[10] = {0}; // Assuming max 10 clients
    // int total_time = 0;
    
    // for (int i = 0; i < execution_history.record_count; i++) {
    //     ExecutionRecord* record = &execution_history.records[i];
    //     int client = record->client_number;
    //     if (client < 10) {
    //         client_time[client] += record->duration;
    //         total_time += record->duration;
    //     }
    // }
    
    // // Print CPU usage percentage for each client
    // for (int i = 1; i < 10; i++) {
    //     if (client_time[i] > 0) {
    //         float percentage = (float)client_time[i] / total_time * 100;
    //         printf("%sClient %d:%s %.1f%% CPU time (%d/%d rounds)\n", 
    //                client_colors[i % NUM_COLORS], i, COLOR_RESET, 
    //                percentage, client_time[i], total_time);
    //     }
    // }
    
    // printf("\n===============================\n\n");
}
