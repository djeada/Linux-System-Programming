#include "../common/io.h"
/* liburing's Linux ABI headers use GNU layouts; keep our code strict C11. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <liburing.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/* Exactly one request in flight. The buffer lives through its completion. */
static int transfer(struct io_uring *ring, int fd, void *buffer,
                    unsigned length, uint64_t offset, bool writing) {
  struct io_uring_sqe *entry = io_uring_get_sqe(ring);
  if (!entry)
    return -EBUSY;
  if (writing)
    io_uring_prep_write(entry, fd, buffer, length, offset);
  else
    io_uring_prep_read(entry, fd, buffer, length, offset);
  int result;
  do {
    result = io_uring_submit(ring);
  } while (result == -EINTR);
  if (result != 1)
    return result < 0 ? result : -EIO;
  struct io_uring_cqe *completion;
  do {
    result = io_uring_wait_cqe(ring, &completion);
  } while (result == -EINTR);
  if (result < 0)
    return result;
  result = completion->res; /* Errors here are negative errno, not errno. */
  io_uring_cqe_seen(ring, completion);
  return result;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s source destination\n", argv[0]);
    return 1;
  }
  struct io_uring ring;
  int error = io_uring_queue_init(8, &ring, 0);
  if (error < 0) {
    fprintf(stderr, "%s: io_uring_queue_init: %s\n",
            error == -EPERM || error == -ENOSYS || error == -EOPNOTSUPP ||
                    error == -EACCES
                ? "SKIP"
                : "ERROR",
            strerror(-error));
    return error == -EPERM || error == -ENOSYS || error == -EOPNOTSUPP ||
                   error == -EACCES
               ? 77
               : 1;
  }
  int in = open(argv[1], O_RDONLY | O_CLOEXEC), out = -1, result = 1;
  if (in < 0)
    goto cleanup;
  out = open(argv[2], O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
  if (out < 0)
    goto cleanup;
  struct stat a, b;
  if (fstat(in, &a) < 0 || fstat(out, &b) < 0)
    goto cleanup;
  if (!S_ISREG(a.st_mode) || !S_ISREG(b.st_mode) || lab_same_file(&a, &b)) {
    errno = EINVAL;
    goto cleanup;
  }
  if (ftruncate(out, 0) < 0)
    goto cleanup;
  unsigned char buffer[65536];
  uint64_t offset = 0;
  for (;;) {
    int count = transfer(&ring, in, buffer, sizeof buffer, offset, false);
    if (count == -EINTR)
      continue;
    if (count < 0) {
      errno = -count;
      goto cleanup;
    }
    if (!count)
      break;
    unsigned done = 0;
    while (done < (unsigned)count) {
      int written = transfer(&ring, out, buffer + done, (unsigned)count - done,
                             offset + done, true);
      if (written == -EINTR)
        continue;
      if (written <= 0) {
        errno = written ? -written : EIO;
        goto cleanup;
      }
      done += (unsigned)written;
    }
    offset += (unsigned)count;
  }
  if (lab_sync(out) < 0)
    goto cleanup;
  printf("copied=%" PRIu64 " using=io_uring\n", offset);
  result = 0;
cleanup:
  if (result)
    perror("io_uring copy");
  io_uring_queue_exit(&ring);
  if (out >= 0 && close(out) < 0) {
    perror("close destination");
    result = 1;
  }
  if (in >= 0)
    close(in);
  return result;
}
