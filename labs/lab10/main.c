#include "../common/io.h"
#include <dirent.h>

/* Every descent is relative to an already-open directory; symlinks are listed.
 */
static int walk(int fd, const char *prefix, unsigned depth) {
  DIR *directory = fdopendir(fd);
  if (!directory) {
    close(fd);
    return -1;
  }
  int result = 0;
  for (;;) {
    errno = 0;
    struct dirent *entry = readdir(directory);
    if (!entry) {
      if (errno)
        result = -1;
      break;
    }
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;
    struct stat info;
    if (fstatat(dirfd(directory), entry->d_name, &info, AT_SYMLINK_NOFOLLOW) <
        0) {
      result = -1;
      break;
    }
    char *path = NULL;
    if (asprintf(&path, "%s%s%s", prefix, *prefix ? "/" : "", entry->d_name) <
        0) {
      result = -1;
      break;
    }
    char type = S_ISDIR(info.st_mode)   ? 'd'
                : S_ISLNK(info.st_mode) ? 'l'
                : S_ISREG(info.st_mode) ? 'f'
                                        : '?';
    printf("%c %04o links=%ju size=%jd %s\n", type,
           (unsigned)(info.st_mode & 07777), (uintmax_t)info.st_nlink,
           (intmax_t)info.st_size, path);
    if (S_ISDIR(info.st_mode)) {
      if (depth >= 64) {
        errno = ELOOP;
        result = -1;
      } else {
        int child = openat(dirfd(directory), entry->d_name,
                           O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (child < 0 || walk(child, path, depth + 1) < 0)
          result = -1;
      }
    }
    free(path);
    if (result < 0)
      break;
  }
  int saved = errno;
  if (closedir(directory) < 0 && !result)
    return -1;
  errno = saved;
  return result;
}

static int replace(const char *target, const char *contents) {
  char *path = strdup(target);
  if (!path)
    return -1;
  char *slash = strrchr(path, '/');
  const char *name = slash ? slash + 1 : path;
  const char *parent = ".";
  if (slash) {
    *slash = '\0';
    parent = *path ? path : "/";
  }
  int directory = -1, in = -1, out = -1, result = -1;
  bool temporary = false;
  char temp[80];
  if (!*name || !strcmp(name, ".") || !strcmp(name, "..")) {
    errno = EINVAL;
    goto done;
  }
  directory = open(parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (directory < 0)
    goto done;
  struct stat existing;
  if (fstatat(directory, name, &existing, AT_SYMLINK_NOFOLLOW) == 0) {
    if (!S_ISREG(existing.st_mode)) {
      errno = EINVAL;
      goto done;
    }
  } else if (errno != ENOENT)
    goto done;
  in = open(contents, O_RDONLY | O_CLOEXEC);
  if (in < 0)
    goto done;
  struct stat source;
  if (fstat(in, &source) < 0)
    goto done;
  if (!S_ISREG(source.st_mode)) {
    errno = EINVAL;
    goto done;
  }
  for (unsigned i = 0; i < 100; ++i) {
    snprintf(temp, sizeof temp, ".lab10-%ld-%u", (long)getpid(), i);
    out =
        openat(directory, temp, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (out >= 0) {
      temporary = true;
      break;
    }
    if (errno != EEXIST)
      goto done;
  }
  if (out < 0)
    goto done;
  uint64_t bytes;
  if (lab_copy(in, out, &bytes) < 0 || lab_sync(out) < 0)
    goto done;
  int closed = close(out);
  out = -1;
  if (closed < 0)
    goto done;
  if (renameat(directory, temp, directory, name) < 0)
    goto done;
  temporary = false;
  if (lab_sync(directory) < 0) {
    fprintf(stderr,
            "Replacement is visible; directory durability is unconfirmed\n");
    goto done;
  }
  printf("replaced=%s bytes=%" PRIu64
         " requested_mode=0600 synced=file+directory\n",
         target, bytes);
  result = 0;
done: {
  int saved = errno;
  if (out >= 0)
    close(out);
  if (temporary)
    unlinkat(directory, temp, 0);
  if (in >= 0)
    close(in);
  if (directory >= 0)
    close(directory);
  free(path);
  errno = saved;
  return result;
}
}

int main(int argc, char **argv) {
  int result;
  if (argc == 3 && !strcmp(argv[1], "list")) {
    int directory =
        open(argv[2], O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    result = directory < 0 ? -1 : walk(directory, "", 0);
  } else if (argc == 4 && !strcmp(argv[1], "replace")) {
    result = replace(argv[2], argv[3]);
  } else {
    fprintf(stderr, "Usage: %s list directory | replace target contents_file\n",
            argv[0]);
    return 1;
  }
  if (result < 0)
    perror("filesystem operation");
  return result < 0;
}
