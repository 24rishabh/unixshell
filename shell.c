#include <stdio.h>     
#include <stdlib.h>    
#include <sys/types.h>
#include <string.h>

#define MAX_ARGS 10

// Things to do
// Parsing
// strip logic
// Argument parsing including quoted strings
// I/O redirection (<, >)
// Built-ins: cd, exit
// Handling ctrl+c
// handle exit -> done

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

int main(){
    // Initialization of buffer and lengths
    char* input = NULL;
    size_t len = 0;
    char *args[MAX_ARGS];
    // get the input from command line
    while(1){
    printf("myshell> ");
    ssize_t nread = getline(&input, &len, stdin);
    // tokenize -> convert and store it into array
    int i = parse_input(input, args);
    if (strcmp(args[0], "exit\n") == 0) {
        break;
    }

    printf("%d\n",i);
    for (int j = 0; args[j] != NULL; j++) {
    printf("arg[%d] = %s\n", j, args[j]);
    }
}

}