#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

void ArgParse(char *input,char *args[]){
    int i = 0;
    char *p = input;
    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (*p == '\"') {
            p++;                  // skip "
            args[i++] = p;        // start
            while (*p != '\"' && *p != '\0') p++;
            if (*p == '\"') {
                *p = '\0';        // terminate
                p++;              // move past "
            }
        } else {
            args[i++] = p;        // start
            while (*p != ' ' && *p != '\t' && *p != '\0') p++;
            if (*p != '\0') {
                *p = '\0';        // terminate
                p++;              // move past space/tab
            }
        }

        if (i >= MAX_ARGS - 1) break;
    }
    args[i] = NULL;
}

int main(){
    char input[MAX_INPUT];
    char* args[MAX_ARGS];
    while(1){
        printf("tsh>");
        fflush(stdout);
        if(fgets(input, MAX_INPUT, stdin)==NULL){
            perror("fgets failed");
            continue;
        }
        input[strcspn(input,"\n")] = '\0';
        if(strcmp(input,"exit")==0){
            printf("Have a nice day!!!\n");
            break;
        }

        ArgParse(input,args);
        if(args[0]==NULL) continue;

        if(strcmp(args[0],"cd")==0){
            if(args[1]==NULL){
                fprintf(stderr, "cd: missing argument\n");
            }
            else if(chdir(args[1])!=0){
                perror("cd failed");
            }
            continue;
        }

        pid_t pid = fork();

        if(pid == 0){
            execvp(args[0],args);
            perror("execvp failure");
            exit(EXIT_FAILURE);
        }
        else if(pid>0){
            int status;
            waitpid(pid, &status, 0);
            printf("Exit Status %d\n",status);
        }
        else{
            perror("fork failed");
        }
    }
    return 0;
}
