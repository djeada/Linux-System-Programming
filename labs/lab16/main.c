#include "../common/io.h"
#include <dirent.h>
#include <sys/resource.h>

static int descriptor_count(void) {
  DIR *directory = opendir("/proc/self/fd");
  if (!directory)
    return -1;
  int count = 0;
  errno = 0;
  struct dirent *entry;
  while ((entry = readdir(directory)))
    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
      ++count;
  int saved = errno;
  closedir(directory);
  if (saved) {
    errno = saved;
    return -1;
  }
  return count - 1; /* Exclude the directory's own descriptor. */
}

/* A named, observable operation suitable for a GDB breakpoint. */
uint64_t checksum_bytes(uint64_t checksum, const unsigned char *bytes,
                        size_t size) {
  for (size_t i = 0; i < size; ++i)
    checksum = checksum * 33 + bytes[i];
  return checksum;
}

int main(int argc, char **argv) {
  int rounds = 3;
  if (argc < 2 || argc > 3 ||
      (argc == 3 && !lab_parse_int(argv[2], 1, &rounds))) {
    fprintf(stderr, "Usage: %s file [rounds=3]\n", argv[0]);
    return 1;
  }
  int before = descriptor_count();
  if (before < 0) {
    perror("/proc/self/fd");
    return 1;
  }
  uint64_t checksum = 0, bytes = 0;
  for (int i = 0; i < rounds; ++i) {
    int fd = open(argv[1], O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
      perror("open");
      return 1;
    }
    unsigned char buffer[4096];
    ssize_t count;
    while ((count = lab_read(fd, buffer, sizeof buffer)) > 0) {
      checksum = checksum_bytes(checksum, buffer, (size_t)count);
      bytes += (uint64_t)count;
    }
    int saved = errno;
    int closed = close(fd);
    if (count < 0 || closed < 0) {
      if (count < 0)
        errno = saved;
      perror("read/close");
      return 1;
    }
  }
  struct rusage usage;
  int after = descriptor_count();
  if (after < 0 || getrusage(RUSAGE_SELF, &usage) < 0) {
    perror("diagnostics");
    return 1;
  }
  printf("bytes=%" PRIu64 " checksum=%" PRIu64
         " fds_before=%d fds_after=%d maxrss_kib=%ld user_us=%ld\n",
         bytes, checksum, before, after, usage.ru_maxrss,
         usage.ru_utime.tv_sec * 1000000L + usage.ru_utime.tv_usec);
  return before != after;
}
