#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<unistd.h>
#include<stdlib.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

int main(){
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    while(1){
        printf("tsh>");
        fflush(stdout);
        // 1.Read a line
        if(fgets(input,MAX_INPUT,stdin)==NULL){
            perror("fgets failed\n");
            continue;
        }

        // Remove trailing \n
        input[strcspn(input,"\n")] = '\0';

        // 2.Built-in: exit
        if(strcmp(input,"exit")==0){
            printf("Byee!!!\n");
            break;
        }

        // 3.Split input into args using strtok (space-separated)
        char *token = strtok(input," ");
        int i = 0;
        while(token!=NULL && i<MAX_ARGS-1){
            args[i++] = token;
            token = strtok(NULL," ");
        }
        args[i] = NULL;

        //3.5 Built-in: cd
        if(strcmp(args[0],"cd")==0){
            if(args[1]==NULL){
                fprintf(stderr,"cd missing arguments\n");
            }
            else if(chdir(args[1])!=0){
                perror("cd");
            }

            continue;
        }
        
        // 4. creating new process using fork
        pid_t pid = fork();
        if(pid==0){
            execvp(args[0],args);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
        else if(pid>0){
            int status;
            waitpid(pid,&status,0);
            printf("exit status: %d\n",status);
        }
        else{
            // fork error
            perror("fork error");
        }
    }
}