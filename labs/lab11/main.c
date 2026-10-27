#include "../common/net.h"

int main(int argc, char **argv) {
  if (argc < 4)
    goto usage;
  bool server = !strcmp(argv[1], "server");
  if (!server && strcmp(argv[1], "client"))
    goto usage;
  bool local = !strcmp(argv[2], "unix"), udp = !strcmp(argv[2], "udp");
  if (!local && !udp && strcmp(argv[2], "tcp"))
    goto usage;
  int base = local ? 4 : 5, requests = 1;
  if ((server && argc != base && argc != base + 1) ||
      (!server && argc != base + 1))
    goto usage;
  if (server && argc == base + 1 && !lab_parse_int(argv[base], 1, &requests))
    goto usage;
  int fd = local ? lab_unix_socket(argv[3], server)
                 : lab_inet_socket(argv[3], argv[4],
                                   udp ? SOCK_DGRAM : SOCK_STREAM, server);
  if (fd < 0) {
    perror("socket setup");
    return 1;
  }
  int result = 1;
  unsigned char buffer[LAB_FRAME_MAX];
  uint32_t size = 0;
  if (server) {
    if ((!udp && listen(fd, 16) < 0) || lab_socket_timeout(fd) < 0) {
      perror("listen/timeout");
      goto cleanup;
    }
    if (local) {
      printf("LISTEN path=%s\n", argv[3]);
      fflush(stdout);
    } else if (lab_print_port(fd) < 0) {
      perror("getsockname");
      goto cleanup;
    }
    for (int i = 0; i < requests; ++i) {
      if (udp) {
        struct sockaddr_storage peer;
        socklen_t length = sizeof peer;
        ssize_t count;
        do {
          count = recvfrom(fd, buffer, LAB_DATAGRAM_MAX, MSG_TRUNC,
                           (struct sockaddr *)&peer, &length);
        } while (count < 0 && errno == EINTR);
        if (count < 0) {
          perror("recvfrom");
          goto cleanup;
        }
        if (count > LAB_DATAGRAM_MAX) {
          errno = EMSGSIZE;
          perror("datagram");
          goto cleanup;
        }
        ssize_t sent;
        do {
          sent = sendto(fd, buffer, (size_t)count, MSG_NOSIGNAL,
                        (struct sockaddr *)&peer, length);
        } while (sent < 0 && errno == EINTR);
        if (sent != count) {
          perror("sendto");
          goto cleanup;
        }
      } else {
        int peer;
        do {
          peer = accept4(fd, NULL, NULL, SOCK_CLOEXEC);
        } while (peer < 0 && errno == EINTR);
        if (peer < 0) {
          perror("accept");
          goto cleanup;
        }
        int error = lab_socket_timeout(peer) < 0 ||
                    lab_receive_frame(peer, buffer, &size) < 0 ||
                    lab_send_frame(peer, buffer, size) < 0;
        if (error)
          perror("frame");
        close(peer);
        if (error)
          goto cleanup;
      }
    }
  } else {
    const char *message = argv[base];
    size_t length = strlen(message);
    if (length > (udp ? LAB_DATAGRAM_MAX : LAB_FRAME_MAX)) {
      fprintf(stderr, "Message too large\n");
      goto cleanup;
    }
    if (lab_socket_timeout(fd) < 0) {
      perror("timeout");
      goto cleanup;
    }
    if (udp) {
      if (send(fd, message, length, MSG_NOSIGNAL) != (ssize_t)length) {
        perror("send");
        goto cleanup;
      }
      ssize_t count;
      do {
        count = recv(fd, buffer, LAB_DATAGRAM_MAX, MSG_TRUNC);
      } while (count < 0 && errno == EINTR);
      if (count < 0 || count > LAB_DATAGRAM_MAX) {
        perror("recv");
        goto cleanup;
      }
      size = (uint32_t)count;
    } else if (lab_send_frame(fd, message, (uint32_t)length) < 0 ||
               lab_receive_frame(fd, buffer, &size) < 0) {
      perror("frame");
      goto cleanup;
    }
    if (lab_write_all(STDOUT_FILENO, buffer, size) < 0 ||
        lab_write_all(STDOUT_FILENO, "\n", 1) < 0) {
      perror("stdout");
      goto cleanup;
    }
  }
  result = 0;
cleanup:
  close(fd);
  if (local && server && unlink(argv[3]) < 0) {
    perror("unlink owned socket");
    result = 1;
  }
  return result;
usage:
  fprintf(stderr,
          "Usage: %s server tcp|udp host port [requests]\n"
          "       %s client tcp|udp host port message\n"
          "       %s server unix path [requests] | client unix path message\n",
          argv[0], argv[0], argv[0]);
  return 1;
}
