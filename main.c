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


        input[strcspn(input,"\n")]= 0;//return the number of characters found in a string before any part of the specified characters are found
        if(strcmp(input,"exit")==0){
            break;
        }

        char *token = strtok(input," ");//splits a string into multiple pieces (referred to as "tokens") using delimiters
        char *args[MAX_ARGS];
        int i = 0;
        int redirect_out = 0;
        int redirect_in=0;
        int fd_out = -1;
        int fd_in=-1;
        while (token!=NULL){
            if(strcmp(token,">")==0){// 0 if the two strings are equal
                redirect_out = 1;
                token = strtok(NULL," "); // get next filename 
                if(token==NULL){
                    fprintf(stderr,"Error: No valid file included \n");
                    redirect_out = 0;
                    break;
                }
                fd_out = open(token,O_CREAT| O_WRONLY | O_TRUNC,0644);
                if (fd_out==-1){
                    perror("Error creating file");
                    break;
                }
                
                break ;
                
            }else if(strcmp(token,"<")==0){
                redirect_in=1;
                token = strtok(NULL," "); // get next filename 
                if(token==NULL){
                    fprintf(stderr,"Error: No valid file included \n");
                    redirect_in = 0;
                    break;
                }
                fd_in=open(token,O_RDONLY);
                if(fd_in==-1){
                    perror("Error in opening input file");
                    break;

                }
                break;
            }
            
            else{
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
    
            if(redirect_out && fd_out!=-1){
                dup2(fd_out,STDOUT_FILENO);
                close(fd_out);
            }
            if(redirect_in && fd_in!=-1){
                dup2(fd_in,STDIN_FILENO);
                close(fd_in);
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