#include "../common/io.h"
#include <sys/mman.h>

int main(int argc, char **argv) {
  uint64_t offset;
  if (argc != 5 || (strcmp(argv[2], "shared") && strcmp(argv[2], "private")) ||
      !lab_u64(argv[3], INT64_MAX, &offset)) {
    fprintf(stderr, "Usage: %s file shared|private offset text\n", argv[0]);
    return 1;
  }
  bool shared = !strcmp(argv[2], "shared");
  int fd = open(argv[1], (shared ? O_RDWR : O_RDONLY) | O_CLOEXEC);
  if (fd < 0) {
    perror("open");
    return 1;
  }
  int result = 1;
  void *mapping = MAP_FAILED;
  size_t size = 0;
  struct flock lock = {.l_type = shared ? F_WRLCK : F_RDLCK,
                       .l_whence = SEEK_SET};
  int locked;
  do {
    locked = fcntl(fd, F_SETLKW, &lock);
  } while (locked < 0 && errno == EINTR);
  if (locked < 0)
    goto cleanup;
  struct stat info;
  if (fstat(fd, &info) < 0)
    goto cleanup;
  if (!S_ISREG(info.st_mode) || info.st_size <= 0 ||
      (uintmax_t)info.st_size > SIZE_MAX || offset > (uint64_t)info.st_size ||
      strlen(argv[4]) > (uint64_t)info.st_size - offset) {
    errno = EINVAL;
    goto cleanup;
  }
  size = (size_t)info.st_size;
  mapping = mmap(NULL, size, PROT_READ | PROT_WRITE,
                 shared ? MAP_SHARED : MAP_PRIVATE, fd, 0);
  if (mapping == MAP_FAILED)
    goto cleanup;
  memcpy((unsigned char *)mapping + offset, argv[4], strlen(argv[4]));
  if (shared && (msync(mapping, size, MS_SYNC) < 0 || lab_sync(fd) < 0))
    goto cleanup;
  if (mprotect(mapping, size, PROT_READ) < 0)
    goto cleanup;
  if (memcmp((unsigned char *)mapping + offset, argv[4], strlen(argv[4]))) {
    errno = EIO;
    goto cleanup;
  }
  printf("mode=%s bytes=%zu page_size=%ld persisted=%d protection=read-only\n",
         argv[2], size, sysconf(_SC_PAGESIZE), shared);
  result = 0;
cleanup:
  if (result)
    perror("mapping");
  if (mapping != MAP_FAILED && munmap(mapping, size) < 0) {
    perror("munmap");
    result = 1;
  }
  if (close(fd) < 0) {
    perror("close");
    result = 1;
  }
  return result;
}
