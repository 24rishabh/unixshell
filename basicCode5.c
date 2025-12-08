#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_PATH 1024

/*
TODO LIST:
1. I/O redirection: < and >
2. Pipelines: cmd1 | cmd2
3. Background: &
4. Ctrl-C handling
*/

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

int main(){
    char input[MAX_INPUT];
    char *args[MAX_ARGS];
    char cwd[MAX_PATH];
    
    while(1){
        if(getcwd(cwd,sizeof(cwd))==NULL){
            perror("cwd failed");
            printf("tsh$ ");
        }
        else{
            printf("%s$ ",cwd);
        }
        fflush(stdout);

        if(fgets(input,MAX_INPUT,stdin)==NULL){
            perror("fgets failed");
            continue;
        }

        input[strcspn(input,"\n")] = '\0';

        if(strcmp(input,"exit")==0){
            printf("Byee!!!\n");
            break;
        }

        // char *token = strtok(input," ");
        // int i = 0;
        // while(input!=NULL && i<MAX_ARGS-1){
        //     args[i++] = token;
        //     token = strtok(NULL," ");
        // }
        // args[i] = NULL;
        ArgParser(input,args);
        if(args[0]==NULL) continue;

        if(strcmp(args[0],"cd")==0){
            if(args[1]==NULL) fprintf(stderr,"cd missing argument\n");
            else if(chdir(args[1])!=0) perror("cd failed");

            continue;
        }

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
            perror("fork failed");
        }
    }

    return 0;
}