#ifndef LAB_NET_H
#define LAB_NET_H
#include "io.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#define LAB_FRAME_MAX 65536u
#define LAB_DATAGRAM_MAX 4096u

static inline int lab_socket_timeout(int fd) {
  struct timeval timeout = {.tv_sec = 3};
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0)
    return -1;
  return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout);
}

static inline int lab_send_all(int fd, const void *data, size_t size) {
  const unsigned char *cursor = data;
  while (size) {
    ssize_t count = send(fd, cursor, size, MSG_NOSIGNAL);
    if (count < 0 && errno == EINTR)
      continue;
    if (count <= 0) {
      if (!count)
        errno = EPIPE;
      return -1;
    }
    cursor += count;
    size -= (size_t)count;
  }
  return 0;
}

static inline int lab_receive_exact(int fd, void *data, size_t size) {
  unsigned char *cursor = data;
  while (size) {
    ssize_t count = recv(fd, cursor, size, 0);
    if (count < 0 && errno == EINTR)
      continue;
    if (count <= 0) {
      if (!count)
        errno = EPROTO;
      return -1;
    }
    cursor += count;
    size -= (size_t)count;
  }
  return 0;
}

static inline int lab_send_frame(int fd, const void *data, uint32_t size) {
  uint32_t header = htonl(size);
  if (size > LAB_FRAME_MAX) {
    errno = EMSGSIZE;
    return -1;
  }
  if (lab_send_all(fd, &header, sizeof header) < 0)
    return -1;
  return lab_send_all(fd, data, size);
}

static inline int lab_receive_frame(int fd, unsigned char *buffer,
                                    uint32_t *size) {
  uint32_t header;
  if (lab_receive_exact(fd, &header, sizeof header) < 0)
    return -1;
  *size = ntohl(header);
  if (*size > LAB_FRAME_MAX) {
    errno = EMSGSIZE;
    return -1;
  }
  return lab_receive_exact(fd, buffer, *size);
}

/* A nonblocking connect plus poll bounds connection setup, too. */
static inline int lab_connect(int fd, const struct sockaddr *address,
                              socklen_t size) {
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    return -1;
  int result = connect(fd, address, size);
  if (result < 0 && errno == EINPROGRESS) {
    struct pollfd ready = {.fd = fd, .events = POLLOUT};
    do {
      result = poll(&ready, 1, 3000);
    } while (result < 0 && errno == EINTR);
    if (!result) {
      errno = ETIMEDOUT;
      return -1;
    }
    if (result < 0)
      return -1;
    int error = 0;
    socklen_t length = sizeof error;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
      return -1;
    if (error) {
      errno = error;
      return -1;
    }
  } else if (result < 0)
    return -1;
  return fcntl(fd, F_SETFL, flags);
}

static inline int lab_inet_socket(const char *host, const char *port, int type,
                                  bool server) {
  struct addrinfo hints = {.ai_family = AF_UNSPEC,
                           .ai_socktype = type,
                           .ai_flags = server ? AI_PASSIVE : 0};
  struct addrinfo *addresses = NULL;
  int error = getaddrinfo(host, port, &hints, &addresses);
  if (error) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
    errno = EINVAL;
    return -1;
  }
  int fd = -1, saved = EADDRNOTAVAIL;
  for (struct addrinfo *p = addresses; p; p = p->ai_next) {
    fd = socket(p->ai_family, type | SOCK_CLOEXEC, p->ai_protocol);
    if (fd < 0) {
      saved = errno;
      continue;
    }
    int yes = 1;
    int result =
        server ? setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) : 0;
    if (!result)
      result = server ? bind(fd, p->ai_addr, p->ai_addrlen)
                      : lab_connect(fd, p->ai_addr, p->ai_addrlen);
    if (!result)
      break;
    saved = errno;
    close(fd);
    fd = -1;
  }
  freeaddrinfo(addresses);
  if (fd < 0)
    errno = saved;
  return fd;
}

static inline int lab_unix_socket(const char *path, bool server) {
  struct sockaddr_un address = {.sun_family = AF_UNIX};
  if (strlen(path) >= sizeof address.sun_path) {
    errno = ENAMETOOLONG;
    return -1;
  }
  strcpy(address.sun_path, path);
  int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (fd < 0)
    return -1;
  /* Do not unlink a pathname that this process does not own. */
  int result =
      server ? bind(fd, (struct sockaddr *)&address, sizeof address)
             : lab_connect(fd, (struct sockaddr *)&address, sizeof address);
  if (result < 0) {
    int saved = errno;
    close(fd);
    errno = saved;
    return -1;
  }
  return fd;
}

static inline int lab_print_port(int fd) {
  struct sockaddr_storage address;
  socklen_t length = sizeof address;
  if (getsockname(fd, (struct sockaddr *)&address, &length) < 0)
    return -1;
  unsigned port = address.ss_family == AF_INET
                      ? ntohs(((struct sockaddr_in *)&address)->sin_port)
                      : ntohs(((struct sockaddr_in6 *)&address)->sin6_port);
  printf("LISTEN port=%u\n", port);
  fflush(stdout);
  return 0;
}
#endif
