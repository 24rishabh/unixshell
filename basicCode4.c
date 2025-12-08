#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdbool.h>   // <-- required for bool, true, false
#include<fcntl.h>     // <-- required for open(), O_RDONLY, etc.

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_PATH 1024

/*
TODO LIST:
1. I/O redirection: < and >  ----DONE----
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

        ArgParser(input,args);
        if(args[0]==NULL) continue;

        if(strcmp(args[0],"cd")==0){
            if(args[1]==NULL) fprintf(stderr,"cd missing argument\n");
            else if(chdir(args[1])!=0) perror("cd failed");

            continue;
        }

        // ----- I/O redirection parsing -----
        int inDirect = 0, outDirect = 0;
        char *infile = NULL;
        char *outfile = NULL;
        char *cleanArgs[MAX_ARGS];
        int j = 0;
        bool parseError = false;

        for(int k=0;args[k]!=NULL;k++){
            if(strcmp(args[k],"<")==0){
                if(args[k+1]==NULL){
                    fprintf(stderr,"Syntax Error: no input file present\n");
                    parseError = true;
                    break;
                }
                inDirect = 1;
                infile = args[k+1];
                k++; // skip the filename
            }
            else if(strcmp(args[k],">")==0){
                if(args[k+1]==NULL){
                    fprintf(stderr,"Syntax Error: no output file present\n");
                    parseError = true;
                }
                outDirect = 1;
                outfile = args[k+1];
                k++; //skip out filename
            }
            else{
                cleanArgs[j++] = args[k];
            }
        }

        if(parseError==true) continue;
        cleanArgs[j] = NULL;

        pid_t pid = fork();
        if(pid==0){
            if(inDirect==1){
                int fd_in = open(infile,O_RDONLY);
                if(fd_in<0){
                    perror("file open error");
                    exit(EXIT_FAILURE);
                }
                if(dup2(fd_in,STDIN_FILENO)<0){
                    perror("dup2 error");
                    close(fd_in);
                    exit(EXIT_FAILURE);
                }
                close(fd_in);
            }
            if(outDirect==1){
                int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out < 0) {
                    perror("open output file");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd_out, STDOUT_FILENO) < 0) {
                    perror("dup2 output");
                    close(fd_out);
                    exit(EXIT_FAILURE);
                }
                close(fd_out);
            }
            execvp(cleanArgs[0],cleanArgs);
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