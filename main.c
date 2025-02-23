#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for fork() and execvp()
#include <sys/wait.h> // for wait()

#define MAX_ARGS 100


int main(){
     char *input = NULL;
     size_t len = 0;
     pid_t child;
    while(1){
       
        printf("$ ");
        ssize_t line = getline(&input,&len,stdin);
        if (line == -1){
            perror("Error reading Input");
            exit(1);
        }


        input[strcspn(input,"\n")]= 0;
        if(strcmp(input,"exit")==0){
            break;
        }

        char *token = strtok(input," ");
        char *args[MAX_ARGS];
        int i = 0;
        while (token!=NULL){
            args[i] = token;
            i++;
            token = strtok(NULL," ");
        }
        args[i] = NULL;
        child = fork();

        if(child ==-1){
            perror("fork failed");
        }

        else if(child ==0){
            
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        }

        else{
            wait(NULL);

        }
        
        
    }
    
    //free(input);
    return 0;
}