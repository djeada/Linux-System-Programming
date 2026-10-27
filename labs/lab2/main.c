/* C11/POSIX: independent file descriptions and explicit child completion. */
#include "../common/system.h"

static int read_file_with_pid(const char *path) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror("fopen");
    return EXIT_FAILURE;
  }
  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;
  while ((length = getline(&line, &capacity, fp)) >= 0) {
    printf("[PID %ld] ", (long)getpid());
    fwrite(line, 1, (size_t)length, stdout);
    if (length && line[length - 1] != '\n')
      putchar('\n');
  }
  int result = ferror(fp) ? EXIT_FAILURE : EXIT_SUCCESS;
  if (result)
    perror("read file");
  free(line);
  if (fclose(fp) != 0) {
    perror("fclose");
    result = EXIT_FAILURE;
  }
  return result;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return EXIT_FAILURE;
  }
  pid_t child = lab_fork();
  if (child < 0) {
    perror("fork");
    return EXIT_FAILURE;
  }
  if (child == 0)
    lab_child_exit(read_file_with_pid(argv[1]));
  int result = read_file_with_pid(argv[1]);
  if (!lab_wait_child(child))
    result = EXIT_FAILURE;
  return result;
}
