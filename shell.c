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
    (void)sig; 
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

// handling pipeline
bool handlePipeline(char *args[], int quoted[], int background) {
    int pipePos = -1;

    // Find first unquoted '|'
    for (int i = 0; args[i] != NULL; i++) {
        if (quoted[i] == 0 && strcmp(args[i], "|") == 0) {
            pipePos = i;
            break;
        }
    }

    if (pipePos == -1) return false; // no pipeline

    if (pipePos == 0 || args[pipePos + 1] == NULL) {
        fprintf(stderr, "Syntax Error: Invalid use of '|'\n");
        return true; // pipeline detected but invalid
    }

    // split into left and right command arrays
    char *leftArgs[MAX_ARGS];
    char *rightArgs[MAX_ARGS];
    int leftQuoted[MAX_ARGS];
    int rightQuoted[MAX_ARGS];

    int li = 0;
    for (int i = 0; i < pipePos && li < MAX_ARGS - 1; i++) {
        leftArgs[li] = args[i];
        leftQuoted[li] = quoted[i];
        li++;
    }
    leftArgs[li] = NULL;
    leftQuoted[li] = 0;

    int ri = 0;
    for (int i = pipePos + 1; args[i] != NULL && ri < MAX_ARGS - 1; i++) {
        rightArgs[ri] = args[i];
        rightQuoted[ri] = quoted[i];
        ri++;
    }
    rightArgs[ri] = NULL;
    rightQuoted[ri] = 0;

    int fds[2];
    if (pipe(fds) < 0) {
        perror("pipe failed");
        return true;
    }

    // Left child -> writes to pipe
    pid_t pid1 = fork();
    if (pid1 == 0) {
        if (dup2(fds[1], STDOUT_FILENO) < 0) {
            perror("dup2 pipe failed for left command");
            exit(EXIT_FAILURE);
        }
        close(fds[0]);
        close(fds[1]);

        handle_redirection(leftArgs, leftQuoted);
        execvp(leftArgs[0], leftArgs);
        perror("execvp pipe failed for left command");
        exit(EXIT_FAILURE);
    } else if (pid1 < 0) {
        perror("fork failed for left command");
        close(fds[0]);
        close(fds[1]);
        return true;
    }

    // Right child -> reads from pipe
    pid_t pid2 = fork();
    if (pid2 == 0) {
        if (dup2(fds[0], STDIN_FILENO) < 0) {
            perror("dup2 pipe failed for right command");
            exit(EXIT_FAILURE);
        }
        close(fds[0]);
        close(fds[1]);

        handle_redirection(rightArgs, rightQuoted);
        execvp(rightArgs[0], rightArgs);
        perror("execvp failed for right command");
        exit(EXIT_FAILURE);
    } else if (pid2 < 0) {
        perror("fork failed for right command");
        close(fds[0]);
        close(fds[1]);
        return true;
    }

    // Parent -> close pipe fds
    close(fds[0]);
    close(fds[1]);

    if (!background) {
        int status;
        waitpid(pid1, &status, 0);
        waitpid(pid2, &status, 0);
    } else {
        printf("[background pipeline] pids: %d, %d\n", pid1, pid2);
    }

    return true;
}

int main() {
    char *input = NULL;
    size_t len = 0;
    char *args[MAX_ARGS];
    int quoted[MAX_ARGS];

    signal(SIGINT, handler);

    while (1) {
        // zoombie
        while (waitpid(-1, NULL, WNOHANG) > 0) {

        }
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

        // Background
        int back = 0;
        if (argc > 0 && quoted[argc - 1] == 0 && strcmp(args[argc - 1], "&") == 0) {
            back = 1;
            args[argc - 1] = NULL;
            argc--;
        }

        // Pipeline handling
        bool piped = handlePipeline(args, quoted, back);
        if (piped) {
            continue;
        }

        // Single command (no |)
        pid_t rc = fork();
        if (rc < 0) {
            perror("fork");
        } else if (rc == 0) {
            // child
            handle_redirection(args, quoted);
            execvp(args[0], args);
            perror("exec failed");
            exit(1);
        } else {
            // parent
            if (back) {
                printf("[Running in background] PID: %d\n", rc);
            } else {
                waitpid(rc, NULL, 0);
            }
        }
    }
    free(input);
    return 0;
}
