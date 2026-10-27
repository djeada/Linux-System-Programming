#ifndef LAB_SHELL_H
#define LAB_SHELL_H

#include "system.h"

static inline int lab_shell(int (*launch)(char **)) {
  char *line = NULL;
  size_t capacity = 0;
  int result = EXIT_SUCCESS;
  for (;;) {
    if (isatty(STDIN_FILENO)) {
      fputs("myshell> ", stdout);
      fflush(stdout);
    }
    ssize_t length = getline(&line, &capacity, stdin);
    if (length < 0) {
      if (errno == EINTR) {
        clearerr(stdin);
        continue;
      }
      if (ferror(stdin)) {
        perror("getline");
        result = EXIT_FAILURE;
      }
      break;
    }
    if (memchr(line, '\0', (size_t)length)) {
      fprintf(stderr, "Command contains a NUL byte\n");
      continue;
    }
    size_t count = 0, argument_capacity = 16;
    char **arguments = calloc(argument_capacity, sizeof(*arguments));
    if (!arguments) {
      perror("calloc arguments");
      result = EXIT_FAILURE;
      break;
    }
    char *save = NULL;
    for (char *word = strtok_r(line, " \t\r\n", &save); word;
         word = strtok_r(NULL, " \t\r\n", &save)) {
      if (count + 1 == argument_capacity) {
        if (argument_capacity > SIZE_MAX / (2 * sizeof(*arguments))) {
          free(arguments);
          free(line);
          return EXIT_FAILURE;
        }
        argument_capacity *= 2;
        char **grown =
            realloc(arguments, argument_capacity * sizeof(*arguments));
        if (!grown) {
          perror("realloc arguments");
          free(arguments);
          free(line);
          return EXIT_FAILURE;
        }
        arguments = grown;
      }
      arguments[count++] = word;
    }
    arguments[count] = NULL;
    if (count == 0) {
      free(arguments);
      continue;
    }
    if (strcmp(arguments[0], "exit") == 0) {
      free(arguments);
      break;
    }
    (void)launch(arguments);
    free(arguments);
  }
  free(line);
  return result;
}

#endif
