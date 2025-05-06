#ifndef TASK_H
#define TASK_H

#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>

// Common definitions
#define BUFFER_SIZE 4096

// Maximum number of execution records per task
#define MAX_EXECUTION_RECORDS 50

// Execution record structure for Gantt chart
typedef struct {
    int start_time;  // When this execution started (in rounds)
    int duration;    // How long this execution lasted (in rounds)
    int client_number; // Which client this execution belongs to
    int task_id;     // Task identifier
} ExecutionRecord;

// Task type enumeration
typedef enum {
    TASK_SHELL,
    TASK_PROGRAM
} TaskType;

// Task structure
typedef struct {
    TaskType type;
    int client_fd;
    int client_number;
    struct sockaddr_in client_addr;
    int remaining_time;
    int execution_rounds;
    time_t arrival_time;
    int preempted;
    int total_executed;
    int initial_n;
    char command[BUFFER_SIZE];  // Fixed-size array for command
    char* output;
    size_t output_size;
    size_t max_output_size;
    int last_quantum;       // Last quantum size used for this task
    
    // Execution history for Gantt chart
    int task_id;           // Unique identifier for this task
    int execution_history_count;
    ExecutionRecord execution_history[MAX_EXECUTION_RECORDS];
} Task;

// Color codes for output
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_RESET "\033[0m"
#define BG_BLUE "\033[44m"

// Task operations
void init_task_output(Task* task);
void append_task_output(Task* task, const char* new_output, size_t length);
void update_task_metrics(Task* task, int executed_time);

#endif /* TASK_H */
