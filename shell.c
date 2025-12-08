#include <stdio.h>     
#include <stdlib.h>    
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_ARGS 10

// Things to do
// Parsing -> done
// strip logic -> done
// Argument parsing including quoted strings -> done
// I/O redirection (<, >) -> done
// Built-ins: cd, exit -> done
// Handling ctrl+c
// handle exit -> done
// improving parsing and strip logic
// handling enter -> done


// strip quotes 
void strip_quotes(char **args) {
    for (int j = 0; args[j]; j++) {
        char *s = args[j];
        size_t len = strlen(s);
        if (len >= 2 && s[0] == '\"' && s[len - 1] == '\"') {
            s[len - 1] = '\0';   
            args[j] = s + 1;     
        }
    }
}
// Parsing
int parse_input(char* input, char** args){
    int i = 0;
    args[i] = input;
    i++;
    int in_quotes = 0;
    for (int j = 0; input[j] != '\0' && i < MAX_ARGS; j++) {
        // Toggle state if we see a quote
        if (input[j] == '"') {
            in_quotes = !in_quotes;
        }
        // Split on space only if we are not in quotes
        else if (input[j] == ' ' && in_quotes == 0) {
            input[j] = '\0';       // Replace space with null terminator
            if (input[j+1] != '\0') {
                args[i] = &input[j+1]; // Point to the character after the space
                i++;
            }
        }
    }
    args[i] = NULL;
    return i;
}

// ctrl + c
void handler(int sig) {
    printf("\n");
}


// handling Input and output redirection
int handle_redirection(char **args) {
    int j = 0;
    while (args[j] != NULL) {
        if (strcmp(args[j], ">") == 0) {
            if (args[j+1] == NULL) {
                fprintf(stderr, "Expected filename after '>'\n");
                return -1;
            }
            int fd = open(args[j+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open failed");
                return -1;
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2 failed");
                close(fd);
                return -1;
            }
            close(fd);
            args[j] = NULL;      // terminate argv here for execvp
        }
        else if (strcmp(args[j], "<") == 0) {
            if (args[j+1] == NULL) {
                fprintf(stderr, "Expected filename after '<'\n");
                return -1;
            }
            int fd = open(args[j+1], O_RDONLY);
            if (fd < 0) {
                perror("open failed");
                return -1;
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2 failed");
                close(fd);
                return -1;
            }
            close(fd);
            args[j] = NULL;      // terminate argv here
        }

        j++;
    }
    return 0;
}



int main(){
    // Initialization of buffer and lengths
    char* input = NULL;
    size_t len = 0;
    char *args[MAX_ARGS];
    // handling ctrl+c
    signal(SIGINT, handler);
    // get the input from command line
    while(1){
    printf("myshell> ");
    ssize_t nread = getline(&input, &len, stdin);
    if (nread == -1) {
            break;
    }
    if (input[nread - 1] == '\n') {
        input[nread - 1] = '\0';
    }
    // tokenize -> convert and store it into array
    int i = parse_input(input, args);
    // strip quotes
    strip_quotes(args);
    if (strcmp(args[0], "exit") == 0) {
        break;
    }
    // handling enter
    if (args[0][0] == '\0') {
        continue;
    }
    // handling cd
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } 
        else if (chdir(args[1]) != 0) {
            perror("cd");
        }
    continue;
    }
    printf("%d\n",i);
    for (int j = 0; args[j] != NULL; j++) {
        printf("args[%d] = %s\n", j, args[j]);
    }
    // checking for redirection function
    // char *args[] = {"somecommand", ">", "out.txt", NULL};

    // if (handle_redirection(args) != 0) {
    //     fprintf(stderr, "handle_redirection failed\n");
    //     return 1;
    // }
    // printf("Hello from redirected stdout!\n");
}

}