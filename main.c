// #include <string.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>  // for fork() and execvp()
// #include <sys/wait.h> // for wait()
// #include <fcntl.h>   // for O_CREAT, O_WRONLY, O_TRUNC

// #define MAX_ARGS 100


// int main(){
//      char *input = NULL;
//      size_t len = 0;
//      pid_t child;
//     while(1){
       
//         printf("$ ");
//         ssize_t line = getline(&input,&len,stdin);
//         if (line == -1){
//             perror("Error reading Input");
//             exit(1);
//         }


//         input[strcspn(input,"\n")]= 0;//return the number of characters found in a string before any part of the specified characters are found
//         if(strcmp(input,"exit")==0){
//             break;
//         }

//         char *token = strtok(input," ");//splits a string into multiple pieces (referred to as "tokens") using delimiters
//         char *args[MAX_ARGS];
//         int i = 0;
//         int redirect_out = 0;
//         int redirect_in=0;
//         int fd_out = -1;
//         int fd_in=-1;
//         while (token!=NULL){
//             if(strcmp(token,">")==0){// 0 if the two strings are equal
//                 redirect_out = 1;
//                 token = strtok(NULL," "); // get next filename 
//                 if(token==NULL){
//                     fprintf(stderr,"Error: No valid file included \n");
//                     redirect_out = 0;
//                     break;
//                 }
//                 fd_out = open(token,O_CREAT| O_WRONLY | O_TRUNC,0644);
//                 if (fd_out==-1){
//                     perror("Error creating file");
//                     break;
//                 }
                
//                 break ;
                
//             }else if(strcmp(token,"<")==0){
//                 redirect_in=1;
//                 token = strtok(NULL," "); // get next filename 
//                 if(token==NULL){
//                     fprintf(stderr,"Error: No valid file included \n");
//                     redirect_in = 0;
//                     break;
//                 }
//                 fd_in=open(token,O_RDONLY);
//                 if(fd_in==-1){
//                     perror("Error in opening input file");
//                     break;

//                 }
//                 break;
//             }
            
//             else{
//                 args[i++] = token;
//             }
            
//             token = strtok(NULL," ");
//         }
//         args[i] = NULL;

//         child = fork();

//         if(child ==-1){
//             perror("fork failed");
//         }

//         else if(child ==0){
    
//             if(redirect_out && fd_out!=-1){
//                 dup2(fd_out,STDOUT_FILENO);
//                 close(fd_out);
//             }
//             if(redirect_in && fd_in!=-1){
//                 dup2(fd_in,STDIN_FILENO);
//                 close(fd_in);
//             }            
//             execvp(args[0], args);
//             perror("execvp failed");
//             exit(1);
//         }

//         else{
//             wait(NULL);
            

//         }
        
        
//     }
    
//     //free(input);
//     return 0;
// }

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_ARGS 100
#define MAX_CMDS 10

// Function to parse a command into arguments and handle redirections
void parse_command(char *cmd, char **args, int *input_fd, int *output_fd, int *error_fd) {
    char *token = strtok(cmd, " ");
    int i = 0;

    while (token != NULL && i < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for input redirection\n");
                return;
            }
            *input_fd = open(token, O_RDONLY);
            if (*input_fd == -1) {
                perror("Error opening input file");
                return;
            }
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for output redirection\n");
                return;
            }
            *output_fd = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (*output_fd == -1) {
                perror("Error creating output file");
                return;
            }
        } else if (strcmp(token, "2>") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) {
                fprintf(stderr, "Error: No file provided for error redirection\n");
                return;
            }
            *error_fd = open(token, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (*error_fd == -1) {
                perror("Error creating error file");
                return;
            }
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

// Function to execute a single command
void execute_command(char *cmd) {
    int input_fd = -1, output_fd = -1, error_fd = -1;
    char *args[MAX_ARGS];

    parse_command(cmd, args, &input_fd, &output_fd, &error_fd);

    if (args[0] == NULL) {
        fprintf(stderr, "Error: Empty command\n");
        return;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) { // Child process
        if (input_fd != -1) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        if (error_fd != -1) {
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
        }

        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else { // Parent process
        wait(NULL);
        if (input_fd != -1) close(input_fd);
        if (output_fd != -1) close(output_fd);
        if (error_fd != -1) close(error_fd);
    }
}

// Function to execute piped commands
void execute_piped_commands(char **commands, int cmd_count) {
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];

    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            exit(1);
        }
    }

    for (int i = 0; i < cmd_count; i++) {
        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork failed");
            exit(1);
        }

        if (pids[i] == 0) { // Child process
            int input_fd = -1, output_fd = -1, error_fd = -1;
            char *args[MAX_ARGS];

            parse_command(commands[i], args, &input_fd, &output_fd, &error_fd);

            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < cmd_count - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            if (input_fd != -1) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (output_fd != -1) {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }
            if (error_fd != -1) {
                dup2(error_fd, STDERR_FILENO);
                close(error_fd);
            }

            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        }
    }

    // Close all pipes in parent
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes
    for (int i = 0; i < cmd_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

int main() {
    char *input = NULL;
    size_t len = 0;

    while (1) {
        printf("$ ");
        ssize_t line = getline(&input, &len, stdin);
        if (line == -1) {
            perror("Error reading input");
            exit(1);
        }

        input[strcspn(input, "\n")] = 0; // Remove newline

        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Split input into commands using '|'
        char *commands[MAX_CMDS];
        int cmd_count = 0;
        char *cmd = strtok(input, "|");

        while (cmd != NULL && cmd_count < MAX_CMDS) {
            // Trim leading and trailing spaces
            while (*cmd == ' ') cmd++;
            char *end = cmd + strlen(cmd) - 1;
            while (end > cmd && *end == ' ') end--;
            *(end + 1) = 0;

            if (strlen(cmd) > 0) {
                commands[cmd_count++] = cmd;
            } else {
                fprintf(stderr, "Error: Empty command between pipes\n");
                cmd_count = 0;
                break;
            }
            cmd = strtok(NULL, "|");
        }

        if (cmd_count == 0) {
            continue; // Skip empty or invalid input
        }

        if (cmd_count == 1) {
            execute_command(commands[0]);
        } else {
            execute_piped_commands(commands, cmd_count);
        }
    }

    free(input);
    return 0;
}
