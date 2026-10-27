#include "../common/process.h"

#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define DEFAULT_LINE_WIDTH 20
#define DEFAULT_INPUT_FILE "input.txt"
#define EOL_MARKER "<EOL>"

typedef struct {
  char data[BUF_SIZE];
  size_t head;
  size_t tail;
  size_t count;
  int done;
  sem_t mutex;
  sem_t items;
  sem_t spaces;
} SharedBuffer;

static const char *g_input_path = DEFAULT_INPUT_FILE;
static int g_line_width = DEFAULT_LINE_WIDTH;

static void reader_process(SharedBuffer *out);
static void transformer_process(SharedBuffer *in, SharedBuffer *out);
static void printer_process(SharedBuffer *in);

static bool buffer_init(SharedBuffer *buffer);
static void buffer_destroy(SharedBuffer *buffer);
static void buffer_push(SharedBuffer *buffer, char ch);
static bool buffer_pop(SharedBuffer *buffer, char *out);
static void buffer_signal_done(SharedBuffer *buffer);

static void safe_sem_wait(sem_t *sem);
static void safe_sem_post(sem_t *sem);
static int parse_positive_int(const char *text, const char *label,
                              int *value_out);

int main(int argc, char *argv[]) {
  if (argc > 3) {
    fprintf(stderr, "Usage: %s [input_file] [line_width]\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (argc >= 2) {
    g_input_path = argv[1];
  }

  if (argc == 3) {
    if (parse_positive_int(argv[2], "line_width", &g_line_width) != 0) {
      return EXIT_FAILURE;
    }
  }

  FILE *input = fopen(g_input_path, "r");
  if (!input) {
    perror("open input");
    return EXIT_FAILURE;
  }
  fclose(input); /* Fail before starting IPC workers for an invalid path. */
  SharedBuffer *buffers =
      mmap(NULL, 2 * sizeof(*buffers), PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (buffers == MAP_FAILED) {
    perror("mmap");
    return EXIT_FAILURE;
  }
  if (!buffer_init(&buffers[0])) {
    munmap(buffers, 2 * sizeof(*buffers));
    return EXIT_FAILURE;
  }
  if (!buffer_init(&buffers[1])) {
    buffer_destroy(&buffers[0]);
    munmap(buffers, 2 * sizeof(*buffers));
    return EXIT_FAILURE;
  }
  pid_t children[3] = {0};
  size_t started = 0;
  bool success = false;
  for (int stage = 0; stage < 3; ++stage) {
    pid_t pid = lab_fork();
    if (pid < 0) {
      perror("fork pipeline");
      lab_terminate_children(children, started);
      goto cleanup;
    }
    if (pid == 0) {
      if (stage == 0)
        reader_process(&buffers[0]);
      else if (stage == 1)
        transformer_process(&buffers[0], &buffers[1]);
      else
        printer_process(&buffers[1]);
      lab_child_exit(ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    children[started++] = pid;
  }
  success = lab_supervise_children(children, started);
cleanup:
  buffer_destroy(&buffers[1]);
  buffer_destroy(&buffers[0]);
  munmap(buffers, 2 * sizeof(*buffers));
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void reader_process(SharedBuffer *out) {
  FILE *fp = fopen(g_input_path, "r");
  if (!fp) {
    perror("reader fopen");
    buffer_signal_done(out);
    lab_child_exit(EXIT_FAILURE);
  }

  int ch;
  const char *marker = EOL_MARKER;
  size_t marker_len = strlen(marker);

  while ((ch = fgetc(fp)) != EOF) {
    if (ch == '\n') {
      for (size_t i = 0; i < marker_len; i++) {
        buffer_push(out, marker[i]);
      }
    } else {
      buffer_push(out, (char)ch);
    }
  }

  bool success = !ferror(fp);
  if (fclose(fp) != 0)
    success = false;
  buffer_signal_done(out);
  if (!success) {
    perror("read input");
    lab_child_exit(EXIT_FAILURE);
  }
}

static void transformer_process(SharedBuffer *in, SharedBuffer *out) {
  bool pending_star = false;
  char ch;
  while (buffer_pop(in, &ch)) {
    if (pending_star) {
      pending_star = false;
      if (ch == '*') {
        buffer_push(out, '#');
        continue;
      }
      buffer_push(out, '*');
    }
    if (ch == '*')
      pending_star = true;
    else
      buffer_push(out, ch);
  }
  if (pending_star)
    buffer_push(out, '*');

  buffer_signal_done(out);
}

static void printer_process(SharedBuffer *in) {
  int count = 0;
  char ch;

  while (buffer_pop(in, &ch)) {
    putchar(ch);
    count++;
    if (count == g_line_width) {
      putchar('\n');
      count = 0;
    }
  }

  if (count > 0) {
    putchar('\n');
  }
}

static bool buffer_init(SharedBuffer *buffer) {
  buffer->head = buffer->tail = buffer->count = 0;
  buffer->done = 0;
  sem_t *semaphores[] = {&buffer->mutex, &buffer->items, &buffer->spaces};
  const unsigned int values[] = {1, 0, BUF_SIZE};
  return lab_sem_initialize(semaphores, values, 3);
}

static void buffer_destroy(SharedBuffer *buffer) {
  sem_destroy(&buffer->mutex);
  sem_destroy(&buffer->items);
  sem_destroy(&buffer->spaces);
}

static void buffer_push(SharedBuffer *buffer, char ch) {
  safe_sem_wait(&buffer->spaces);
  safe_sem_wait(&buffer->mutex);

  buffer->data[buffer->tail] = ch;
  buffer->tail = (buffer->tail + 1) % BUF_SIZE;
  buffer->count++;

  safe_sem_post(&buffer->mutex);
  safe_sem_post(&buffer->items);
}

static bool buffer_pop(SharedBuffer *buffer, char *out) {
  safe_sem_wait(&buffer->items);
  safe_sem_wait(&buffer->mutex);

  if (buffer->count == 0 && buffer->done) {
    safe_sem_post(&buffer->mutex);
    return false;
  }

  *out = buffer->data[buffer->head];
  buffer->head = (buffer->head + 1) % BUF_SIZE;
  buffer->count--;

  safe_sem_post(&buffer->mutex);
  safe_sem_post(&buffer->spaces);
  return true;
}

static void buffer_signal_done(SharedBuffer *buffer) {
  safe_sem_wait(&buffer->mutex);
  buffer->done = 1;
  safe_sem_post(&buffer->mutex);
  safe_sem_post(&buffer->items);
}

static void safe_sem_wait(sem_t *sem) {
  while (sem_wait(sem) == -1) {
    if (errno == EINTR) {
      continue;
    }
    perror("sem_wait");
    lab_child_exit(EXIT_FAILURE);
  }
}

static void safe_sem_post(sem_t *sem) {
  if (sem_post(sem) == -1) {
    perror("sem_post");
    lab_child_exit(EXIT_FAILURE);
  }
}

static int parse_positive_int(const char *text, const char *label,
                              int *value_out) {
  char *end = NULL;
  errno = 0;
  long value = strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value <= 0 ||
      value > INT_MAX) {
    fprintf(stderr, "Invalid %s: %s\n", label, text);
    return -1;
  }
  *value_out = (int)value;
  return 0;
}
