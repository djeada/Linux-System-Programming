#ifndef LAB_SYSTEM_H
#define LAB_SYSTEM_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* pthread functions return error numbers; they do not set errno. */
static inline int lab_thread_error(int error, const char *operation) {
  if (error != 0)
    fprintf(stderr, "%s: %s\n", operation, strerror(error));
  return error;
}

static inline void lab_thread_check(int error, const char *operation) {
  if (lab_thread_error(error, operation) != 0)
    abort();
}

static inline bool lab_parse_int(const char *text, int minimum, int *value) {
  char *end = NULL;
  errno = 0;
  long parsed = strtol(text, &end, 10);
  if (errno || end == text || *end || parsed < minimum || parsed > INT_MAX)
    return false;
  *value = (int)parsed;
  return true;
}

static inline struct timespec lab_now(void) {
  struct timespec result;
  if (clock_gettime(CLOCK_MONOTONIC, &result) < 0) {
    perror("clock_gettime");
    abort();
  }
  return result;
}

static inline bool lab_before(struct timespec left, struct timespec right) {
  return left.tv_sec < right.tv_sec ||
         (left.tv_sec == right.tv_sec && left.tv_nsec < right.tv_nsec);
}

static inline void lab_sleep_ms(long milliseconds) {
  struct timespec delay = {milliseconds / 1000,
                           (milliseconds % 1000) * 1000000};
  while (nanosleep(&delay, &delay) < 0) {
    if (errno != EINTR) {
      perror("nanosleep");
      abort();
    }
  }
}

/* Override simulated work only; deadlines and reporting clocks remain real. */
static inline void lab_simulate_ms(long milliseconds) {
  const char *setting = getenv("LAB_DELAY_MS");
  int override = 0;
  if (setting && lab_parse_int(setting, 0, &override))
    milliseconds = override;
  lab_sleep_ms(milliseconds);
}

static inline pid_t lab_fork(void) {
  fflush(NULL); /* Do not duplicate buffered parent output in children. */
  return fork();
}

static inline void lab_child_exit(int status) {
  if (fflush(NULL) == EOF)
    status = EXIT_FAILURE;
  _exit(status); /* Skip inherited parent exit handlers and cleanup paths. */
}

static inline bool lab_wait_child(pid_t pid) {
  int status = 0;
  pid_t result;
  do {
    result = waitpid(pid, &status, 0);
  } while (result < 0 && errno == EINTR);
  if (result < 0) {
    perror("waitpid");
    return false;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    fprintf(stderr, "Child %ld failed\n", (long)pid);
    return false;
  }
  return true;
}

static inline void lab_terminate_children(pid_t *children, size_t count) {
  for (size_t i = 0; i < count; ++i)
    if (children[i] > 0)
      kill(children[i], SIGTERM);
  for (size_t i = 0; i < count; ++i) {
    if (children[i] <= 0)
      continue;
    int status;
    while (waitpid(children[i], &status, 0) < 0 && errno == EINTR) {
    }
    children[i] = 0;
  }
}

/* These standalone process labs own all children. Reap whichever exits first
   so a failed stage cannot leave its peers waiting forever. */
static inline bool lab_supervise_children(pid_t *children, size_t count) {
  for (size_t finished = 0; finished < count; ++finished) {
    int status = 0;
    pid_t pid;
    do {
      pid = waitpid(-1, &status, 0);
    } while (pid < 0 && errno == EINTR);
    if (pid < 0) {
      perror("waitpid");
      lab_terminate_children(children, count);
      return false;
    }
    for (size_t i = 0; i < count; ++i)
      if (children[i] == pid)
        children[i] = 0;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      fprintf(stderr, "Child %ld failed; stopping remaining workers\n",
              (long)pid);
      lab_terminate_children(children, count);
      return false;
    }
  }
  return true;
}

static inline char *lab_trim(char *text) {
  while (isspace((unsigned char)*text))
    ++text;
  char *end = text + strlen(text);
  while (end > text && isspace((unsigned char)end[-1]))
    --end;
  *end = '\0';
  return text;
}

/* Exact, unquoted comma-separated fields; preserve/reject empty fields. */
static inline bool lab_fields(char *line, char **fields, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    char *comma = strchr(line, ',');
    if ((i + 1 < count) != (comma != NULL))
      return false;
    if (comma)
      *comma = '\0';
    fields[i] = lab_trim(line);
    if (!*fields[i])
      return false;
    if (comma)
      line = comma + 1;
  }
  return true;
}

#endif
