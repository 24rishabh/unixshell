#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdbool.h>  
#include<fcntl.h>   

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_PATH 1024

void ArgParser(char *input,char *args[]){
    char *p = input; // walking pointer through input
    int i = 0; // index into args

    while(*p!='\0'){
        while(*p==' ' || *p=='\t') p++; // skkiping the whitespaces
        if(*p=='\0') break;

        //if argument starts with double quote
        if(*p=='\"'){
            p++;
            args[i++] = p;
            // move until closing qute found
            while(*p!='\"' && *p!='\0') p++;

            if(*p=='\"'){
                *p = '\0';
                p++;
            }
        }
        else{
            // if the argument is normal word
            args[i++] = p;
            while(*p!=' ' && *p!='\t' && *p!='\0') p++;
            if(*p!='\0'){
                *p = '\0';
                p++;
            }
        }

        if(i>=MAX_ARGS-1) break;
    }
    args[i] = NULL;
}

bool handlePipeline(char *args[],bool background){
    int pipePos = -1;
    for(int i=0;args[i]!=NULL;i++){
        if(strcmp(args[i],"|")==0){
            pipePos = i;
            break;
        }
    }

    if(pipePos==-1) return false;
    if(pipePos==0 || args[pipePos+1]==NULL){
        fprintf(stderr,"Syntax Error: Invalid use of '|'\n");
        return true;// pipeline detected but invalid
    }
    // Split args into leftArgs (before '|') and rightArgs (after '|')
    char *leftArgs[MAX_ARGS];
    char *rightArgs[MAX_ARGS];
    int li = 0;
    for(int i=0;i<pipePos;i++){
        leftArgs[li++] = args[i];
    }
    leftArgs[li] = NULL;
    int ri = 0;
    for(int i=pipePos+1;args[i]!=NULL;i++){
        rightArgs[ri++] = args[i];
    }
    rightArgs[ri] = NULL;

    int fds[2];
    if(pipe(fds)<0){
        perror("pipe failed");
        return true; // pipeline detected but pipe failed
    }

    pid_t pid1 = fork();
    if(pid1==0){
        if(dup2(fds[1],STDOUT_FILENO)<0){
            perror("dup2 pipe failed for left command");
            exit(EXIT_FAILURE);
        }
        close(fds[0]); // not used in this child
        close(fds[1]);
        execvp(leftArgs[0],leftArgs);
        perror("execvp pipe failed for left command");
        exit(EXIT_FAILURE);
    }
    else if(pid1<0){
        perror("fork failed for left command");
        close(fds[0]);
        close(fds[1]);
        return true;
    }

    pid_t pid2 = fork();
    if(pid2==0){
        if(dup2(fds[0],STDIN_FILENO)<0){
            perror("dup2 pipe failed for right command");
            exit(EXIT_FAILURE);
        }
        close(fds[0]);
        close(fds[1]); // not used in this child
        execvp(rightArgs[0],rightArgs);
        perror("execvp failed for right command");
        exit(EXIT_FAILURE);
    }
    else if(pid2<0){
        perror("fork failed for right command");
        close(fds[0]);
        close(fds[1]);
        return true;
    }

    //parent closes both the fds
    close(fds[0]);
    close(fds[1]);

    if(background==false){
        // wait for both child to finish
        int status;
        waitpid(pid1,&status,0);
        waitpid(pid2,&status,0);
    }
    else{
        // background pipeline: do not wait here
        printf("[background pipeline] pids: %d, %d\n", pid1, pid2);
    }
    return true; // pipelining handled successfully.
}

bool backgroundDetect(char *args[]){
    int last = 0;
    while(args[last]!=NULL) last++;
    if(last>0 && strcmp(args[last-1],"&")==0){
        args[last-1] = NULL;
        return true;
    }
    return false;
}
