#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define FILE_PATH "/path/to/your/file.txt" // <<<=== Change this path as needed.
#define MAX_LINE_LENGTH 256

static int read_file_with_pid(const char *path) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror("fopen");
    return EXIT_FAILURE;
  }

  char line[MAX_LINE_LENGTH];
  while (fgets(line, sizeof(line), fp)) {
    printf("[PID %ld] %s", (long)getpid(), line);
  }

  fclose(fp);
  return EXIT_SUCCESS;
}

int main(void) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return EXIT_FAILURE;
  }

  return read_file_with_pid(FILE_PATH);
}
