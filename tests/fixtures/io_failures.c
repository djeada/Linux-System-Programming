#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Interpose only a test executable's data descriptors, never Python's I/O. */
static int short_io(int fd, size_t *size, int *calls) {
  if (fd < 3 || !getenv("LAB_SHORT_IO"))
    return 0;
  if (++*calls == 1) {
    errno = EINTR;
    return -1;
  }
  if (*size > 7)
    *size = 7;
  return 0;
}
ssize_t read(int fd, void *buffer, size_t size) {
  static int calls;
  if (short_io(fd, &size, &calls))
    return -1;
  ssize_t (*original)(int, void *, size_t) = dlsym(RTLD_NEXT, "read");
  return original(fd, buffer, size);
}
ssize_t write(int fd, const void *buffer, size_t size) {
  static int calls;
  if (fd >= 3 && getenv("LAB_FAIL_WRITE")) {
    errno = ENOSPC;
    return -1;
  }
  if (short_io(fd, &size, &calls))
    return -1;
  ssize_t (*original)(int, const void *, size_t) = dlsym(RTLD_NEXT, "write");
  return original(fd, buffer, size);
}
ssize_t pread(int fd, void *buffer, size_t size, off_t offset) {
  static int calls;
  if (short_io(fd, &size, &calls))
    return -1;
  ssize_t (*original)(int, void *, size_t, off_t) = dlsym(RTLD_NEXT, "pread");
  return original(fd, buffer, size, offset);
}
ssize_t pwrite(int fd, const void *buffer, size_t size, off_t offset) {
  static int calls;
  if (short_io(fd, &size, &calls))
    return -1;
  ssize_t (*original)(int, const void *, size_t, off_t) =
      dlsym(RTLD_NEXT, "pwrite");
  return original(fd, buffer, size, offset);
}
int fsync(int fd) {
  static int calls;
  const char *setting = getenv("LAB_FAIL_SYNC");
  if (setting && ++calls == atoi(setting)) {
    errno = EIO;
    return -1;
  }
  int (*original)(int) = dlsym(RTLD_NEXT, "fsync");
  return original(fd);
}

ssize_t send(int fd, const void *buffer, size_t size, int flags) {
  static int calls;
  if (getenv("LAB_SHORT_SEND")) {
    ++calls;
    if (calls == 1) {
      errno = EINTR;
      return -1;
    }
    if (calls % 3 == 0) {
      errno = EAGAIN;
      return -1;
    }
    if (size > 37)
      size = 37;
  }
  ssize_t (*original)(int, const void *, size_t, int) =
      dlsym(RTLD_NEXT, "send");
  return original(fd, buffer, size, flags);
}
