/* API COMPARISON: explicit fork/exec is appropriate for custom child setup.
   Prefer ../main.c for a direct program launch. Both use the same parser.
   This fork example has a single-threaded parent and checks child completion.
 */
#include "../../common/shell.h"

static int launch(char **arguments) {
  pid_t child = lab_fork();
  if (child < 0) {
    perror("fork");
    return EXIT_FAILURE;
  }
  if (child == 0) {
    execvp(arguments[0], arguments);
    /* Failed exec must not run inherited parent stdio/exit cleanup. */
    const char message[] = "execvp failed\n";
    ssize_t ignored = write(STDERR_FILENO, message, sizeof(message) - 1);
    (void)ignored;
    _exit(127);
  }
  return lab_wait_child(child) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(void) { return lab_shell(launch); }
