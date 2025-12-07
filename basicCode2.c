#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

int main(){
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    while(1){
        printf("tsh>");
        fflush(stdout); // make sure prompt appears.

        //1.Read a line
        if(fgets(input,MAX_INPUT,stdin)==NULL){
            perror("write the correct prompt");
            continue;
        }

         // Remove trailing '\n'
        input[strcspn(input,"\n")] = '\0';

        // 2. Built-in: exit
        if(strcmp(input,"exit")==0){
            printf("Byee!!!\n");
            break;
        }

        // 3. Split input into args using strtok (space-separated)
        char *token = strtok(input," ");// first token
        int i = 0;
        while(token!=NULL && i<MAX_ARGS-1){
            args[i++] = token;           // store pointer to this word
            token = strtok(NULL," ");  // next token
        }
        args[i] = NULL;   // add NULL at the last
        if(args[0]==NULL) continue; //if no commands then just skip

        // 4. fork a child to run a command
        pid_t pid = fork();
        if(pid==0){ // child process
            execvp(args[0],args);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
        else if(pid>0){ // if parent process present in the shell
            int status;
            waitpid(pid,&status,0); // wait for child to finish
            printf("Exit Status %d\n",status);
        }
        else{
            // fork error
            perror("fork error");
        }
    }
}