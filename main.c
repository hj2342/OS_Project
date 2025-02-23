#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for fork() and execvp()
#include <sys/wait.h> // for wait()
#include <fcntl.h>   // for O_CREAT, O_WRONLY, O_TRUNC

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
        int redirect = 0;
        int fd = -1;
        while (token!=NULL){
            if(strcmp(token,">")==0){
                redirect = 1;
                token = strtok(NULL," "); // get next filename 
                if(token==NULL){
                    fprintf(stderr,"Error: No valid file included \n");
                    redirect = 0;
                    break;
                }
                fd = open(token,O_CREAT| O_WRONLY | O_TRUNC,0644);
                if (fd==-1){
                    perror("Error creating file");
                    break;
                }
                
                break ;
                
            }else{
                args[i++] = token;
            }
            
            token = strtok(NULL," ");
        }
        args[i] = NULL;

        child = fork();

        if(child ==-1){
            perror("fork failed");
        }

        else if(child ==0){
    
            if(redirect && fd!=-1){
                dup2(fd,STDOUT_FILENO);
                close(fd);
            }
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