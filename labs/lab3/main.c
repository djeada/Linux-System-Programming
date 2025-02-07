#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>

#define MAX_LINE 1024   // Maximum length of the input line
#define MAX_ARGS 100    // Maximum number of arguments in a command

/**
 * printPrompt - Displays the shell prompt.
 */
void printPrompt() {
    printf("myshell> ");
    fflush(stdout);
}

/**
 * readCommand - Reads a full line of input from the user.
 * Returns a pointer to a dynamically allocated string that must be freed.
 */
char *readCommand() {
    char *line = malloc(MAX_LINE);
    if (!line) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        free(line);
        return NULL;
    }
    return line;
}

/**
 * tokenizeCommand - Splits the command string into an array of tokens.
 * Returns a NULL-terminated array of pointers.
 */
char **tokenizeCommand(char *line) {
    char **tokens = malloc(sizeof(char *) * (MAX_ARGS + 1));
    if (!tokens) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    
    int position = 0;
    char *token = strtok(line, " \t\r\n");
    while (token != NULL && position < MAX_ARGS) {
        tokens[position] = token;
        position++;
        token = strtok(NULL, " \t\r\n");
    }
    tokens[position] = NULL;  // Terminate the array
    return tokens;
}

int main(void) {
    char *line;
    char **args;
    pid_t pid;
    int status;

    // Main shell loop
    while (1) {
        printPrompt();

        // Read a command line from the user.
        line = readCommand();
        if (line == NULL) {
            // If end-of-file is reached (Ctrl+D), exit the shell.
            printf("\nExiting myshell.\n");
            break;
        }

        // Tokenize the input line.
        args = tokenizeCommand(line);

        // Check if there is any command (empty input).
        if (args[0] == NULL) {
            free(args);
            free(line);
            continue;
        }

        // If the user types "exit", then quit the shell.
        // NOTE: Remember to type a space after exit if needed.
        if (strcmp(args[0], "exit") == 0) {
            free(args);
            free(line);
            printf("Exiting myshell.\n");
            break;
        }

        // Fork a child process.
        pid = fork();
        if (pid < 0) {
            perror("fork");
            free(args);
            free(line);
            continue;
        }

        if (pid == 0) {
            // In the child process: execute the command using execvp.
            if (execvp(args[0], args) == -1) {
                perror("execvp");
            }
            exit(EXIT_FAILURE);  // Exit if execvp fails.
        } else {
            // In the parent process: wait for the child to finish.
            do {
                if (waitpid(pid, &status, 0) == -1) {
                    perror("waitpid");
                    break;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        free(args);
        free(line);
    }

    return EXIT_SUCCESS;
}
