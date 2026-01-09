#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024 // Maximum length of the input line
#define MAX_ARGS 100  // Maximum number of arguments in a command

static void print_prompt(void) {
  printf("myshell> ");
  fflush(stdout);
}

static char *read_command(void) {
  char *line = malloc(MAX_LINE);
  if (!line) {
    perror("malloc");
    return NULL;
  }
  if (!fgets(line, MAX_LINE, stdin)) {
    free(line);
    return NULL;
  }
  return line;
}

static void trim_trailing_newline(char *line) {
  size_t len = strlen(line);
  if (len == 0) {
    return;
  }
  if (line[len - 1] == '\n') {
    line[len - 1] = '\0';
  }
}

static bool line_is_whitespace_only(const char *line) {
  for (size_t i = 0; line[i] != '\0'; i++) {
    if (!isspace((unsigned char)line[i])) {
      return false;
    }
  }
  return true;
}

static int tokenize_command(char *line, char **argv, int argv_size) {
  int argc = 0;
  char *saveptr = NULL;
  char *token = strtok_r(line, " \t\r\n", &saveptr);

  while (token && argc < argv_size - 1) {
    argv[argc++] = token;
    token = strtok_r(NULL, " \t\r\n", &saveptr);
  }

  argv[argc] = NULL;
  return argc;
}

int main(void) {
  int status = 0;

  while (1) {
    print_prompt();

    char *line = read_command();
    if (!line) {
      printf("\nExiting myshell.\n");
      break;
    }

    trim_trailing_newline(line);
    if (line_is_whitespace_only(line)) {
      free(line);
      continue;
    }

    char *args[MAX_ARGS + 1];
    int argc = tokenize_command(line, args, MAX_ARGS + 1);

    if (argc == 0) {
      free(line);
      continue;
    }

    if (strcmp(args[0], "exit") == 0) {
      free(line);
      printf("Exiting myshell.\n");
      break;
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      free(line);
      continue;
    }

    if (pid == 0) {
      execvp(args[0], args);
      perror("execvp");
      exit(EXIT_FAILURE);
    }

    do {
      if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        break;
      }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    free(line);
  }

  return EXIT_SUCCESS;
}
