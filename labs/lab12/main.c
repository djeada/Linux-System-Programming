#include "../common/net.h"
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>

enum { CAPACITY = 64, LISTENER = 0, TIMER = 1, SIGNALS = 2, FIRST_CLIENT = 3 };
typedef struct {
  int fd;
  uint32_t generation;
  unsigned char bytes[4 + LAB_FRAME_MAX];
  size_t have, need, sent;
  bool writing;
  uint64_t active;
} Client;

static uint64_t token(unsigned slot, uint32_t generation) {
  return ((uint64_t)generation << 32) | (slot + FIRST_CLIENT);
}
static int watch(int epoll, int operation, int fd, uint32_t events,
                 uint64_t id) {
  struct epoll_event event = {.events = events, .data.u64 = id};
  return epoll_ctl(epoll, operation, fd, &event);
}
static void disconnect(int epoll, Client *client) {
  epoll_ctl(epoll, EPOLL_CTL_DEL, client->fd, NULL);
  close(client->fd);
  client->fd = -1;
}

/* One bounded frame per session. Reading pauses while a response is pending. */
static bool progress(int epoll, Client *client, unsigned slot) {
  while (!client->writing) {
    ssize_t count = recv(client->fd, client->bytes + client->have,
                         client->need - client->have, 0);
    if (count < 0 && errno == EINTR)
      continue;
    if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      return true;
    if (count <= 0)
      return false;
    client->have += (size_t)count;
    client->active = lab_milliseconds();
    if (client->have != client->need)
      continue;
    if (client->need == 4) {
      uint32_t header;
      memcpy(&header, client->bytes, 4);
      uint32_t length = ntohl(header);
      if (length > LAB_FRAME_MAX)
        return false;
      client->need = 4 + length;
      if (length)
        continue;
    }
    client->writing = true;
  }
  while (client->sent < client->need) {
    ssize_t count = send(client->fd, client->bytes + client->sent,
                         client->need - client->sent, MSG_NOSIGNAL);
    if (count < 0 && errno == EINTR)
      continue;
    if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      return watch(epoll, EPOLL_CTL_MOD, client->fd, EPOLLOUT,
                   token(slot, client->generation)) == 0;
    }
    if (count <= 0)
      return false;
    client->sent += (size_t)count;
    client->active = lab_milliseconds();
  }
  return false;
}

int main(int argc, char **argv) {
  int port, limit = 0, idle = 3000;
  if (argc < 2 || argc > 4 || !lab_parse_int(argv[1], 0, &port) ||
      port > 65535 || (argc > 2 && !lab_parse_int(argv[2], 0, &limit)) ||
      (argc > 3 && !lab_parse_int(argv[3], 1, &idle))) {
    fprintf(stderr, "Usage: %s port [max_sessions=0] [idle_ms=3000]\n",
            argv[0]);
    return 1;
  }
  sigset_t mask, saved;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  if (sigprocmask(SIG_BLOCK, &mask, &saved) < 0) {
    perror("sigprocmask");
    return 1;
  }
  Client *clients = calloc(CAPACITY, sizeof *clients);
  if (!clients) {
    sigprocmask(SIG_SETMASK, &saved, NULL);
    return 1;
  }
  for (unsigned i = 0; i < CAPACITY; ++i)
    clients[i].fd = -1;
  int listener = -1, epoll = -1, timer = -1, signals = -1, result = 1;
  listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (listener < 0)
    goto cleanup;
  int reuse = 1;
  if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse) < 0)
    goto cleanup;
  struct sockaddr_in address = {.sin_family = AF_INET,
                                .sin_port = htons((uint16_t)port),
                                .sin_addr.s_addr = htonl(INADDR_LOOPBACK)};
  if (bind(listener, (struct sockaddr *)&address, sizeof address) < 0 ||
      listen(listener, 64) < 0)
    goto cleanup;
  epoll = epoll_create1(EPOLL_CLOEXEC);
  timer = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  signals = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
  if (epoll < 0 || timer < 0 || signals < 0)
    goto cleanup;
  struct itimerspec interval = {.it_interval = {.tv_nsec = 50000000},
                                .it_value = {.tv_nsec = 50000000}};
  if (timerfd_settime(timer, 0, &interval, NULL) < 0 ||
      watch(epoll, EPOLL_CTL_ADD, listener, EPOLLIN, LISTENER) < 0 ||
      watch(epoll, EPOLL_CTL_ADD, timer, EPOLLIN, TIMER) < 0 ||
      watch(epoll, EPOLL_CTL_ADD, signals, EPOLLIN, SIGNALS) < 0 ||
      lab_print_port(listener) < 0)
    goto cleanup;
  bool accepting = true, registered = true;
  uint64_t stop_at = UINT64_MAX;
  unsigned active = 0, accepted = 0;
  while (accepting || active) {
    bool want_listener = accepting && active < CAPACITY;
    if (want_listener != registered) {
      if (watch(epoll, want_listener ? EPOLL_CTL_ADD : EPOLL_CTL_DEL, listener,
                EPOLLIN, LISTENER) < 0)
        goto cleanup;
      registered = want_listener;
    }
    struct epoll_event events[32];
    int count = epoll_wait(epoll, events, 32, -1);
    if (count < 0 && errno == EINTR)
      continue;
    if (count < 0)
      goto cleanup;
    for (int i = 0; i < count; ++i) {
      uint64_t id = events[i].data.u64;
      unsigned key = (unsigned)id;
      if (key == LISTENER && accepting) {
        while (active < CAPACITY && accepting) {
          int fd = accept4(listener, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
          if (fd < 0 && errno == EINTR)
            continue;
          if (fd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
          if (fd < 0)
            goto cleanup;
          unsigned slot = 0;
          while (clients[slot].fd >= 0)
            ++slot;
          uint32_t generation = clients[slot].generation + 1;
          clients[slot] = (Client){.fd = fd,
                                   .generation = generation,
                                   .need = 4,
                                   .active = lab_milliseconds()};
          if (watch(epoll, EPOLL_CTL_ADD, fd, EPOLLIN | EPOLLRDHUP,
                    token(slot, generation)) < 0)
            goto cleanup;
          ++active;
          ++accepted;
          if (limit && accepted >= (unsigned)limit)
            accepting = false;
        }
      } else if (key == SIGNALS) {
        struct signalfd_siginfo info;
        while (read(signals, &info, sizeof info) == sizeof info) {
        }
        accepting = false;
        uint64_t requested = lab_milliseconds() + (unsigned)idle;
        if (requested < stop_at)
          stop_at = requested;
      } else if (key == TIMER) {
        uint64_t ticks;
        if (read(timer, &ticks, sizeof ticks) < 0 && errno != EAGAIN)
          goto cleanup;
        uint64_t now = lab_milliseconds();
        for (unsigned j = 0; j < CAPACITY; ++j)
          if (clients[j].fd >= 0 &&
              (now - clients[j].active >= (unsigned)idle || now >= stop_at)) {
            disconnect(epoll, &clients[j]);
            --active;
          }
      } else if (key >= FIRST_CLIENT && key < FIRST_CLIENT + CAPACITY) {
        unsigned slot = key - FIRST_CLIENT;
        Client *client = &clients[slot];
        /* Ignore queued notifications for a previous occupant of this slot. */
        if (client->fd >= 0 && client->generation == (uint32_t)(id >> 32) &&
            !progress(epoll, client, slot)) {
          disconnect(epoll, client);
          --active;
        }
      }
    }
  }
  printf("sessions=%u drained\n", accepted);
  result = 0;
cleanup:
  if (result)
    perror("event server");
  for (unsigned i = 0; i < CAPACITY; ++i)
    if (clients[i].fd >= 0)
      close(clients[i].fd);
  free(clients);
  if (signals >= 0)
    close(signals);
  if (timer >= 0)
    close(timer);
  if (epoll >= 0)
    close(epoll);
  if (listener >= 0)
    close(listener);
  sigprocmask(SIG_SETMASK, &saved, NULL);
  return result;
}
