#include "../common/io.h"

/* Positional I/O leaves both descriptors' current offsets unchanged. */
static int copy_range(int in, int out, uint64_t offset, uint64_t length,
                      uint64_t *total) {
  unsigned char buffer[65536];
  *total = 0;
  while (*total < length) {
    size_t wanted = length - *total < sizeof buffer ? (size_t)(length - *total)
                                                    : sizeof buffer;
    ssize_t got;
    do {
      got = pread(in, buffer, wanted, (off_t)(offset + *total));
    } while (got < 0 && errno == EINTR);
    if (got < 0)
      return -1;
    if (!got)
      break;
    size_t done = 0;
    while (done < (size_t)got) {
      ssize_t n = pwrite(out, buffer + done, (size_t)got - done,
                         (off_t)(*total + done));
      if (n < 0 && errno == EINTR)
        continue;
      if (n <= 0) {
        if (!n)
          errno = EIO;
        return -1;
      }
      done += (size_t)n;
    }
    *total += (uint64_t)got;
  }
  return 0;
}

int main(int argc, char **argv) {
  uint64_t offset = 0, length = 0;
  if ((argc != 3 && argc != 5) ||
      (argc == 5 && (!lab_u64(argv[3], INT64_MAX, &offset) ||
                     !lab_u64(argv[4], INT64_MAX - offset, &length)))) {
    fprintf(stderr, "Usage: %s source destination [offset length]\n", argv[0]);
    return 1;
  }
  int in = open(argv[1], O_RDONLY | O_CLOEXEC);
  if (in < 0) {
    perror("open source");
    return 1;
  }
  int out = open(argv[2], O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600);
  int result = 1;
  struct stat a, b;
  if (out < 0) {
    perror("open destination");
    goto cleanup;
  }
  if (fstat(in, &a) < 0 || fstat(out, &b) < 0) {
    perror("fstat");
    goto cleanup;
  }
  if (!S_ISREG(a.st_mode) || !S_ISREG(b.st_mode) || lab_same_file(&a, &b)) {
    fprintf(stderr, "Source and destination must be different regular files\n");
    goto cleanup;
  }
  if (ftruncate(out, 0) < 0) {
    perror("ftruncate");
    goto cleanup;
  }
  uint64_t bytes;
  if ((argc == 5 ? copy_range(in, out, offset, length, &bytes)
                 : lab_copy(in, out, &bytes)) < 0) {
    perror("copy");
    goto cleanup;
  }
  printf("copied=%" PRIu64, bytes);
  if (argc == 5)
    printf(" source_offset=%jd destination_offset=%jd",
           (intmax_t)lseek(in, 0, SEEK_CUR), (intmax_t)lseek(out, 0, SEEK_CUR));
  putchar('\n');
  result = 0;
cleanup:
  /* On Linux close releases the descriptor even on EINTR: never retry it. */
  if (out >= 0 && close(out) < 0) {
    perror("close destination");
    result = 1;
  }
  if (close(in) < 0) {
    perror("close source");
    result = 1;
  }
  return result;
}
