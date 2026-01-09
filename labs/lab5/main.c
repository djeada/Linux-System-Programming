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

static void buffer_init(SharedBuffer *buffer);
static void buffer_destroy(SharedBuffer *buffer);
static void buffer_push(SharedBuffer *buffer, char ch);
static bool buffer_pop(SharedBuffer *buffer, char *out);
static void buffer_signal_done(SharedBuffer *buffer);

static void safe_sem_wait(sem_t *sem);
static void safe_sem_post(sem_t *sem);
static void wait_for_child(pid_t pid);
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

  SharedBuffer *shm1 = mmap(NULL, sizeof(*shm1), PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shm1 == MAP_FAILED) {
    perror("mmap shm1");
    return EXIT_FAILURE;
  }

  SharedBuffer *shm2 = mmap(NULL, sizeof(*shm2), PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shm2 == MAP_FAILED) {
    perror("mmap shm2");
    return EXIT_FAILURE;
  }

  buffer_init(shm1);
  buffer_init(shm2);

  pid_t reader_pid = fork();
  if (reader_pid < 0) {
    perror("fork reader");
    return EXIT_FAILURE;
  }
  if (reader_pid == 0) {
    reader_process(shm1);
    exit(EXIT_SUCCESS);
  }

  pid_t transformer_pid = fork();
  if (transformer_pid < 0) {
    perror("fork transformer");
    return EXIT_FAILURE;
  }
  if (transformer_pid == 0) {
    transformer_process(shm1, shm2);
    exit(EXIT_SUCCESS);
  }

  pid_t printer_pid = fork();
  if (printer_pid < 0) {
    perror("fork printer");
    return EXIT_FAILURE;
  }
  if (printer_pid == 0) {
    printer_process(shm2);
    exit(EXIT_SUCCESS);
  }

  wait_for_child(reader_pid);
  wait_for_child(transformer_pid);
  wait_for_child(printer_pid);

  buffer_destroy(shm1);
  buffer_destroy(shm2);
  munmap(shm1, sizeof(*shm1));
  munmap(shm2, sizeof(*shm2));

  return EXIT_SUCCESS;
}

static void reader_process(SharedBuffer *out) {
  FILE *fp = fopen(g_input_path, "r");
  if (!fp) {
    perror("reader fopen");
    buffer_signal_done(out);
    exit(EXIT_FAILURE);
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

  fclose(fp);
  buffer_signal_done(out);
}

static void transformer_process(SharedBuffer *in, SharedBuffer *out) {
  char prev = '\0';
  char ch;

  while (buffer_pop(in, &ch)) {
    if (prev == '*') {
      if (ch == '*') {
        buffer_push(out, '#');
        prev = '\0';
        continue;
      }
      buffer_push(out, prev);
      prev = ch;
      continue;
    }

    if (ch == '*') {
      prev = ch;
    } else {
      buffer_push(out, ch);
    }
  }

  if (prev == '*') {
    buffer_push(out, prev);
  }

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

static void buffer_init(SharedBuffer *buffer) {
  buffer->head = 0;
  buffer->tail = 0;
  buffer->count = 0;
  buffer->done = 0;

  if (sem_init(&buffer->mutex, 1, 1) == -1 ||
      sem_init(&buffer->items, 1, 0) == -1 ||
      sem_init(&buffer->spaces, 1, BUF_SIZE) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }
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
    exit(EXIT_FAILURE);
  }
}

static void safe_sem_post(sem_t *sem) {
  if (sem_post(sem) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
}

static void wait_for_child(pid_t pid) {
  while (waitpid(pid, NULL, 0) == -1) {
    if (errno == EINTR) {
      continue;
    }
    perror("waitpid");
    break;
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
