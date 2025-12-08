#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_ARGS  10

// Things to do
// Parsing -> done
// strip logic -> done
// Argument parsing including quoted strings -> done
// I/O redirection (<, >) -> done
// Built-ins: cd, exit -> done
// Handling ctrl+c -> done
// handle exit -> done
// handling enter -> done


volatile sig_atomic_t is_waiting_for_input = 0;

// handling ctrl+c
void handler(int sig) {
    const char msg_nl[] = "\n";
    write(STDOUT_FILENO, msg_nl, sizeof(msg_nl) - 1);

    if (is_waiting_for_input) {
        const char msg_prompt[] = "myshell> ";
        write(STDOUT_FILENO, msg_prompt, sizeof(msg_prompt) - 1);
    }
}

// parsing
int parse_input(char *input, char **args, int *quoted) {
    int argc = 0;
    char *p = input;

    while (*p != '\0' && argc < MAX_ARGS) {
        // Skip spaces and tabs
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') break;

        // quoted
        if (*p == '"') {
            p++;  // opening
            args[argc] = p;
            quoted[argc] = 1;

            while (*p != '\0' && *p != '"') {
                p++;
            }
            if (*p == '"') {
                *p = '\0';  // terminate arg
                p++;        // move past closing "
            }
            argc++;
        }
        //normal unquoted argument
        else {
            args[argc] = p;
            quoted[argc] = 0;

            while (*p != '\0' && *p != ' ' && *p != '\t') {
                p++;
            }
            if (*p != '\0') {
                *p = '\0';  // terminate arg
                p++;        // move past '\0'
            }
            argc++;
        }
    }

    args[argc] = NULL;
    return argc;
}

// Input and output redirection
void handle_redirection(char **args, int *quoted) {
    for (int j = 0; args[j] != NULL; j++) {
        // Skip tokens that were originally quoted
        if (quoted[j] == 0 && strcmp(args[j], ">") == 0) {
            if (args[j + 1] == NULL) {
                fprintf(stderr, "Expected filename after '>'\n");
                exit(1);
            }
            int fd = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2 failed");
                close(fd);
                exit(1);
            }
            close(fd);
            args[j] = NULL;
            break;
        }
        else if (quoted[j] == 0 && strcmp(args[j], "<") == 0) {
            if (args[j + 1] == NULL) {
                fprintf(stderr, "Expected filename after '<'\n");
                exit(1);
            }
            int fd = open(args[j + 1], O_RDONLY);
            if (fd < 0) {
                perror("open failed");
                exit(1);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2 failed");
                close(fd);
                exit(1);
            }
            close(fd);
            args[j] = NULL;
            break;
        }
    }
}

int main() {
    char *input = NULL;
    size_t len = 0;
    char *args[MAX_ARGS];
    int quoted[MAX_ARGS];

    signal(SIGINT, handler);

    while (1) {
        printf("myshell> ");

        is_waiting_for_input = 1;
        // taking input
        ssize_t nread = getline(&input, &len, stdin);

        is_waiting_for_input = 0;
        // ctrl+d
        if (nread == -1) {
            break;
        }

        if (nread > 0 && input[nread - 1] == '\n') {
            input[nread - 1] = '\0';
        }
        // parsing
        int argc = parse_input(input, args, quoted);
        if (argc == 0 || args[0] == NULL || args[0][0] == '\0') {
            continue;
        }
        // handling exit
        if (strcmp(args[0], "exit") == 0) {
            break;
        }
        // handling cd
        if (strcmp(args[0], "cd") == 0) {
            if (argc < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else if (chdir(args[1]) != 0) {
                perror("cd");
            }
            continue;
        }

    return 0;
}
