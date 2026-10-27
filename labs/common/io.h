#ifndef LAB_IO_H
#define LAB_IO_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "system.h"
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>

static inline ssize_t lab_read(int fd, void *data, size_t size) {
  ssize_t count;
  do {
    count = read(fd, data, size);
  } while (count < 0 && errno == EINTR);
  return count;
}

static inline int lab_write_all(int fd, const void *data, size_t size) {
  const unsigned char *cursor = data;
  while (size) {
    ssize_t count = write(fd, cursor, size);
    if (count < 0 && errno == EINTR)
      continue;
    if (count <= 0) {
      if (!count)
        errno = EIO;
      return -1;
    }
    cursor += count;
    size -= (size_t)count;
  }
  return 0;
}

static inline int lab_copy(int source, int destination, uint64_t *bytes) {
  unsigned char buffer[65536];
  *bytes = 0;
  for (;;) {
    ssize_t count = lab_read(source, buffer, sizeof buffer);
    if (count < 0)
      return -1;
    if (!count)
      return 0;
    if (lab_write_all(destination, buffer, (size_t)count) < 0)
      return -1;
    *bytes += (uint64_t)count;
  }
}

static inline bool lab_u64(const char *text, uint64_t maximum,
                           uint64_t *value) {
  if (!*text || *text == '-' || isspace((unsigned char)*text))
    return false;
  char *end;
  errno = 0;
  uintmax_t parsed = strtoumax(text, &end, 10);
  if (errno || *end || parsed > maximum)
    return false;
  *value = (uint64_t)parsed;
  return true;
}

static inline int lab_sync(int fd) {
  int result;
  do {
    result = fsync(fd);
  } while (result < 0 && errno == EINTR);
  return result;
}

static inline bool lab_same_file(const struct stat *a, const struct stat *b) {
  return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

static inline uint64_t lab_milliseconds(void) {
  struct timespec now = lab_now();
  return (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
}
#endif
