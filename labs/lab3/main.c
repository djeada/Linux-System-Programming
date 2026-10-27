/* Recommended program launch when no custom child work is needed. */
#include "../common/shell.h"
#include <spawn.h>

extern char **environ;

static int launch(char **arguments) {
  pid_t child = -1;
  int error =
      posix_spawnp(&child, arguments[0], NULL, NULL, arguments, environ);
  if (error != 0) {
    fprintf(stderr, "%s: %s\n", arguments[0], strerror(error));
    return EXIT_FAILURE;
  }
  return lab_wait_child(child) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(void) { return lab_shell(launch); }
