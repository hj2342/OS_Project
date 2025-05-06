#include "task_executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int calculate_quantum(const Task* task) {
    if (!task) return 0;
    return (task->execution_rounds == 0) ? FIRST_QUANTUM : OTHER_QUANTUM;
}

int should_preempt(const Task* current, const Task* shortest) {
    if (!current || !shortest) return 0;
    if (current == shortest) return 0;
    
    // Only preempt program tasks
    if (current->type != TASK_PROGRAM || shortest->type != TASK_PROGRAM) {
        return 0;
    }
    
    return (current->remaining_time - shortest->remaining_time) > PREEMPTION_THRESHOLD;
}

void execute_program_task(Task* task, int quantum) {
    printf("\033[34m[%d]--- created (%d)\033[0m\n", 
           task->client_number, task->remaining_time);

    printf("\033[32m[%d]--- started (%d)\033[0m\n", 
           task->client_number, task->remaining_time);

    int to_execute = (task->remaining_time < quantum) ? 
                    task->remaining_time : quantum;
    
    size_t total_bytes = 0;
    for (int i = 0; i < to_execute && task->remaining_time > 0; i++) {
        printf("\033[33m[%d]--- running (%d)\033[0m\n", 
               task->client_number, task->remaining_time);

        // Send progress to client
        char progress[50];
        int current_step = task->total_executed + i + 1;
        snprintf(progress, sizeof(progress), "Demo %d/%d\n", 
                 current_step,
                 task->initial_n);  // Use stored initial N value
        ssize_t bytes_written = write(task->client_fd, progress, strlen(progress));
        if (bytes_written > 0) {
            total_bytes += bytes_written;
        }
        
        sleep(1);
        task->remaining_time--;
    }

    task->total_executed += to_execute;
    task->execution_rounds++;

    if (task->remaining_time > 0) {
        printf("\033[35m[%d]--- waiting (%d)\033[0m\n", 
               task->client_number, task->remaining_time);
        // Send preemption status to client
        char status[64];
        snprintf(status, sizeof(status), "[STATUS] PREEMPTED %d/%d\n",
                task->total_executed,
                task->remaining_time + task->total_executed);
        write(task->client_fd, status, strlen(status));
    } else {
        // Print bytes sent message before ending
        printf("[%d]<<< %zu bytes sent\n", task->client_number, total_bytes);
        printf("\033[31m[%d]--- ended (0)\033[0m\n", 
               task->client_number);
        // Send completion status to client
        char status[64];
        snprintf(status, sizeof(status), "[STATUS] COMPLETED\n");
        write(task->client_fd, status, strlen(status));
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
        
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        size_t total_bytes = 0;
        
        // Read and send output to client
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            write(task->client_fd, buffer, bytes_read);
            total_bytes += bytes_read;
        }
        
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        
        // Print bytes sent message on server side
        printf("[%d]<<< %zu bytes sent\n", task->client_number, total_bytes); 
        printf("%s[%d]--- ended (-1)%s\n", 
               COLOR_RED, task->client_number, COLOR_RESET);
    }
}
