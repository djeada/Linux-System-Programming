#include "../common/net.h"
#include <stddef.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>

typedef struct {
  int interval_ms;
  char message[512];
} Config;

static int load_config(const char *path, Config *config) {
  FILE *file = fopen(path, "re");
  if (!file)
    return -1;
  Config candidate = {.interval_ms = 1000, .message = "hello"};
  char *line = NULL;
  size_t capacity = 0;
  ssize_t count;
  int result = 0;
  bool interval_seen = false, message_seen = false;
  while ((count = getline(&line, &capacity, file)) >= 0) {
    if (memchr(line, '\0', (size_t)count)) {
      result = -1;
      break;
    }
    char *field = lab_trim(line);
    if (!*field || *field == '#')
      continue;
    char *equals = strchr(field, '=');
    if (!equals) {
      result = -1;
      break;
    }
    *equals++ = '\0';
    char *key = lab_trim(field), *value = lab_trim(equals);
    if (!strcmp(key, "interval_ms") && !interval_seen) {
      interval_seen = true;
      if (!lab_parse_int(value, 1, &candidate.interval_ms))
        result = -1;
    } else if (!strcmp(key, "message") && !message_seen &&
               strlen(value) < sizeof candidate.message) {
      message_seen = true;
      strcpy(candidate.message, value);
    } else
      result = -1;
    if (result)
      break;
  }
  if (ferror(file))
    result = -1;
  free(line);
  if (fclose(file) == EOF)
    result = -1;
  if (result) {
    errno = EINVAL;
    return -1;
  }
  *config = candidate;
  return 0;
}

static int set_timer(int timer, int ms) {
  struct itimerspec interval = {
      .it_value = {.tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L}};
  interval.it_interval = interval.it_value;
  return timerfd_settime(timer, 0, &interval, NULL);
}

/* Implement the small documented notification protocol, without a library. */
static int notify_manager(const char *message) {
  const char *path = getenv("NOTIFY_SOCKET");
  if (!path || !*path)
    return 0;
  struct sockaddr_un address = {.sun_family = AF_UNIX};
  size_t length = strlen(path);
  if (length >= sizeof address.sun_path || (*path != '/' && *path != '@')) {
    errno = EINVAL;
    return -1;
  }
  memcpy(address.sun_path, path, length + 1);
  if (*path == '@')
    address.sun_path[0] = '\0';
  socklen_t size = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + length +
                               (*path == '/' ? 1 : 0));
  int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  if (fd < 0)
    return -1;
  ssize_t sent = sendto(fd, message, strlen(message), MSG_NOSIGNAL,
                        (struct sockaddr *)&address, size);
  int saved = errno;
  close(fd);
  errno = saved;
  return sent == (ssize_t)strlen(message) ? 0 : -1;
}

int main(int argc, char **argv) {
  int maximum = 0;
  if (argc < 2 || argc > 3 ||
      (argc == 3 && !lab_parse_int(argv[2], 1, &maximum))) {
    fprintf(stderr, "Usage: %s config_file [max_ticks]\n", argv[0]);
    return 1;
  }
  Config config;
  if (load_config(argv[1], &config) < 0) {
    perror("configuration");
    return 1;
  }
  sigset_t mask, saved;
  sigemptyset(&mask);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGHUP);
  if (sigprocmask(SIG_BLOCK, &mask, &saved) < 0) {
    perror("sigprocmask");
    return 1;
  }
  int signals = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
  int timer = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
  int result = 1;
  if (signals < 0 || timer < 0 || set_timer(timer, config.interval_ms) < 0 ||
      notify_manager("READY=1\nSTATUS=Configuration loaded; event loop ready") <
          0)
    goto cleanup;
  printf("event=ready interval_ms=%d\n", config.interval_ms);
  fflush(stdout);
  uint64_t ticks = 0;
  bool running = true;
  while (running) {
    struct pollfd ready[] = {{.fd = signals, .events = POLLIN},
                             {.fd = timer, .events = POLLIN}};
    int count = poll(ready, 2, -1);
    if (count < 0 && errno == EINTR)
      continue;
    if (count < 0)
      goto cleanup;
    if (ready[0].revents & POLLIN) {
      struct signalfd_siginfo info;
      while (read(signals, &info, sizeof info) == sizeof info) {
        if (info.ssi_signo == SIGHUP) {
          Config candidate;
          if (load_config(argv[1], &candidate) < 0)
            fprintf(stderr, "event=reload_rejected: %s\n", strerror(errno));
          else {
            if (set_timer(timer, candidate.interval_ms) < 0)
              goto cleanup;
            config = candidate;
            printf("event=reloaded interval_ms=%d message=%s\n",
                   config.interval_ms, config.message);
          }
        } else
          running = false;
      }
    }
    if (running && (ready[1].revents & POLLIN)) {
      uint64_t expirations;
      ssize_t got = read(timer, &expirations, sizeof expirations);
      if (got < 0 && errno == EAGAIN)
        continue; /* A reload reset the timer. */
      if (got != sizeof expirations)
        goto cleanup;
      ++ticks; /* One log per wake; report coalesced timer expirations. */
      printf("event=tick tick=%" PRIu64 " expirations=%" PRIu64 " message=%s\n",
             ticks, expirations, config.message);
      if (maximum && ticks >= (unsigned)maximum)
        running = false;
    }
    fflush(stdout);
  }
  if (notify_manager("STOPPING=1\nSTATUS=Draining complete") < 0)
    perror("stopping notification");
  printf("event=stopped ticks=%" PRIu64 "\n", ticks);
  result = 0;
cleanup:
  if (result)
    perror("service");
  if (timer >= 0)
    close(timer);
  if (signals >= 0)
    close(signals);
  sigprocmask(SIG_SETMASK, &saved, NULL);
  return result;
}
