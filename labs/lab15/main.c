#include "../common/io.h"
#include <grp.h>
#include <linux/audit.h>
#include <linux/capability.h>
#include <linux/filter.h>
#include <linux/magic.h>
#include <linux/seccomp.h>
#include <sched.h>
#include <stddef.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/statfs.h>
#include <sys/syscall.h>

enum { SKIP = 77 };
static int skip(const char *what) {
  fprintf(stderr, "SKIP: %s: %s\n", what, strerror(errno));
  return SKIP;
}
static int show_file(const char *path) {
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return -1;
  uint64_t bytes;
  int result = lab_copy(fd, STDOUT_FILENO, &bytes);
  close(fd);
  return result;
}
static int write_setting(int directory, const char *name, const char *value) {
  int fd = openat(directory, name, O_WRONLY | O_CLOEXEC);
  if (fd < 0)
    return -1;
  int result = lab_write_all(fd, value, strlen(value));
  int saved = errno;
  if (close(fd) < 0 && !result)
    return -1;
  errno = saved;
  return result;
}
static int child_result(pid_t pid) {
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  int status;
  pid_t result;
  do {
    result = waitpid(pid, &status, 0);
  } while (result < 0 && errno == EINTR);
  if (result < 0 || !WIFEXITED(status)) {
    fprintf(stderr, "Child failed\n");
    return 1;
  }
  return WEXITSTATUS(status);
}

static int inspect(void) {
  printf("uid=%ld euid=%ld gid=%ld egid=%ld\n", (long)getuid(), (long)geteuid(),
         (long)getgid(), (long)getegid());
  int count = getgroups(0, NULL);
  if (count < 0)
    return 1;
  gid_t *groups = calloc((size_t)count + 1, sizeof *groups);
  if (!groups || getgroups(count, groups) < 0) {
    free(groups);
    return 1;
  }
  printf("supplementary_groups=");
  for (int i = 0; i < count; ++i)
    printf("%s%ld", i ? "," : "", (long)groups[i]);
  printf("\n");
  free(groups);
  FILE *file = fopen("/proc/self/status", "re");
  if (!file)
    return 1;
  char *line = NULL;
  size_t size = 0;
  while (getline(&line, &size, file) >= 0)
    if (!strncmp(line, "Cap", 3) || !strncmp(line, "NoNewPrivs:", 11) ||
        !strncmp(line, "Seccomp:", 8))
      fputs(line, stdout);
  int result = ferror(file) ? 1 : 0;
  free(line);
  fclose(file);
  return result;
}

static int limits_child(void) {
  struct rlimit limit;
  if (getrlimit(RLIMIT_NOFILE, &limit) < 0)
    return 1;
  if (limit.rlim_cur > 32)
    limit.rlim_cur = 32;
  if (setrlimit(RLIMIT_NOFILE, &limit) < 0)
    return 1;
  int fds[32], count = 0;
  while (count < 32) {
    int fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
      break;
    fds[count++] = fd;
  }
  int error = errno;
  for (int i = 0; i < count; ++i)
    close(fds[i]);
  printf("child soft_limit=%ju opened=%d errno=%s\n", (uintmax_t)limit.rlim_cur,
         count, strerror(error));
  return error == EMFILE ? 0 : 1;
}

static int sandbox_child(void) {
#if defined(__x86_64__) || defined(__aarch64__)
#if defined(__x86_64__)
  const uint32_t architecture = AUDIT_ARCH_X86_64;
#else
  const uint32_t architecture = AUDIT_ARCH_AARCH64;
#endif
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0)
    return skip("no_new_privs");
  struct __user_cap_header_struct header = {.version =
                                                _LINUX_CAPABILITY_VERSION_3};
  struct __user_cap_data_struct data[2] = {0};
  if (syscall(SYS_capset, &header, data) < 0)
    return skip("drop capabilities");
  if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_CLEAR_ALL, 0, 0, 0) < 0 &&
      errno != EINVAL)
    return skip("ambient capabilities");
  struct sock_filter rules[] = {
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, arch)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, architecture, 1, 0),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
#if defined(__x86_64__)
      /* Reject the x32 ABI rather than letting it bypass syscall matching. */
      BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, 0x40000000, 0, 1),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
#endif
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_getppid, 0, 1),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
  };
  struct sock_fprog program = {
      .len = (unsigned short)(sizeof rules / sizeof *rules), .filter = rules};
  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) < 0)
    return skip("seccomp filter");
  errno = 0;
  long result = syscall(SYS_getppid);
  int error = errno;
  printf("child capabilities_dropped=1 no_new_privs=1 getppid=%ld errno=%s\n",
         result, strerror(error));
  return result == -1 && error == EPERM ? 0 : 1;
#else
  errno = ENOTSUP;
  return skip("seccomp example supports x86_64/aarch64");
#endif
}

static int namespaces_child(uid_t uid, gid_t gid) {
  if (unshare(CLONE_NEWUSER | CLONE_NEWUTS) < 0)
    return skip("user/UTS namespaces restricted by host policy");
  int proc = open("/proc/self", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (proc < 0)
    return 1;
  char mapping[80];
  snprintf(mapping, sizeof mapping, "0 %lu 1\n", (unsigned long)uid);
  int result = write_setting(proc, "uid_map", mapping);
  if (!result && write_setting(proc, "setgroups", "deny\n") < 0 &&
      errno != ENOENT)
    result = -1;
  snprintf(mapping, sizeof mapping, "0 %lu 1\n", (unsigned long)gid);
  if (!result)
    result = write_setting(proc, "gid_map", mapping);
  close(proc);
  if (result < 0 || sethostname("lab15-isolated", 14) < 0)
    return skip("namespace mapping/hostname");
  char hostname[256];
  if (gethostname(hostname, sizeof hostname) < 0)
    return 1;
  hostname[sizeof hostname - 1] = '\0';
  printf("child namespace_hostname=%s uid=%ld\n", hostname, (long)getuid());
  return strcmp(hostname, "lab15-isolated") != 0;
}

static int cgroup(const char *parent) {
  if (!parent)
    return show_file("/proc/self/cgroup") < 0;
  int directory = open(parent, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  if (directory < 0)
    return skip("delegated cgroup directory");
  struct statfs fs;
  if (fstatfs(directory, &fs) < 0 || fs.f_type != CGROUP2_SUPER_MAGIC) {
    close(directory);
    errno = EINVAL;
    return skip("directory must be on cgroup v2");
  }
  char name[80];
  snprintf(name, sizeof name, "lab15-%ld", (long)getpid());
  if (mkdirat(directory, name, 0700) < 0) {
    close(directory);
    return skip("cgroup delegation is required");
  }
  int group =
      openat(directory, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  int gate[2] = {-1, -1}, result = 1;
  pid_t child = -1;
  if (group < 0)
    goto cleanup;
  if (write_setting(group, "pids.max", "8\n") < 0) {
    result = skip("delegated pids controller");
    goto cleanup;
  }
  if (write_setting(group, "memory.max", "33554432\n") < 0 && errno != ENOENT) {
    result = skip("delegated memory controller");
    goto cleanup;
  }
  if (pipe2(gate, O_CLOEXEC) < 0)
    goto cleanup;
  child = lab_fork();
  if (child < 0)
    goto cleanup;
  if (!child) {
    close(gate[1]);
    char release;
    if (lab_read(gate[0], &release, 1) != 1)
      _exit(1);
    close(gate[0]);
    close(group);
    close(directory);
    printf("child cgroup membership:\n");
    fflush(stdout);
    lab_child_exit(show_file("/proc/self/cgroup") < 0);
  }
  char pid[40];
  snprintf(pid, sizeof pid, "%ld\n", (long)child);
  if (write_setting(group, "cgroup.procs", pid) < 0) {
    result = skip("moving own child into delegated cgroup");
    goto cleanup;
  }
  if (lab_write_all(gate[1], "x", 1) < 0)
    goto cleanup;
  result = child_result(child);
  child = -1;
  printf("owned_cgroup=%s/%s pids.max=8\n", parent, name);
cleanup:
  if (child > 0) {
    kill(child, SIGKILL);
    child_result(child);
  }
  if (gate[0] >= 0)
    close(gate[0]);
  if (gate[1] >= 0)
    close(gate[1]);
  if (group >= 0)
    close(group);
  if (unlinkat(directory, name, AT_REMOVEDIR) < 0) {
    perror("remove owned cgroup");
    result = 1;
  }
  close(directory);
  return result;
}

int main(int argc, char **argv) {
  if (argc == 2 && !strcmp(argv[1], "inspect"))
    return inspect();
  if ((argc == 2 || argc == 3) && !strcmp(argv[1], "cgroup"))
    return cgroup(argc == 3 ? argv[2] : NULL);
  if (argc != 2)
    goto usage;
  bool limits = !strcmp(argv[1], "limits"),
       sandbox = !strcmp(argv[1], "sandbox"),
       ns = !strcmp(argv[1], "namespaces");
  if (!limits && !sandbox && !ns)
    goto usage;
  struct rlimit before;
  char hostname[256], after[256];
  if (getrlimit(RLIMIT_NOFILE, &before) < 0 ||
      gethostname(hostname, sizeof hostname) < 0)
    return 1;
  uid_t uid = getuid();
  gid_t gid = getgid();
  pid_t child = lab_fork();
  if (!child)
    lab_child_exit(limits    ? limits_child()
                   : sandbox ? sandbox_child()
                             : namespaces_child(uid, gid));
  int result = child_result(child);
  struct rlimit current;
  if (getrlimit(RLIMIT_NOFILE, &current) < 0 ||
      gethostname(after, sizeof after) < 0)
    return 1;
  if (current.rlim_cur != before.rlim_cur ||
      strncmp(hostname, after, sizeof hostname)) {
    fprintf(stderr, "Parent isolation check failed\n");
    return 1;
  }
  printf("parent limits_and_hostname_unchanged=1\n");
  return result;
usage:
  fprintf(stderr,
          "Usage: %s inspect|limits|sandbox|namespaces|cgroup "
          "[delegated_directory]\n",
          argv[0]);
  return 1;
}
