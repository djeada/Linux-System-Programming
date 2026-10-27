# Lab 11: TCP, UDP, and Unix-domain sockets

Build small blocking echo peers before studying event-driven I/O. See
[networking notes](../../notes/networking_and_event_loops.md).

```sh
make build/lab11
# Terminal 1 (one request by default; use port 0 to request an ephemeral port):
./build/lab11 server tcp 127.0.0.1 8080 1
# Terminal 2:
./build/lab11 client tcp 127.0.0.1 8080 'hello world'
```

Replace `tcp` with `udp` for datagrams. For local streams:

```sh
./build/lab11 server unix /tmp/lab11-echo.sock
./build/lab11 client unix /tmp/lab11-echo.sock 'local message'
```

The server prints and flushes `LISTEN port=...` or `LISTEN path=...` when ready.
TCP and Unix streams use a four-byte big-endian unsigned length followed by
at most 65,536 payload bytes. Empty frames and binary payloads are valid;
CLI messages are strings. Each connection carries exactly one request/response.
UDP echoes one datagram of at most 4,096 bytes without a length header.
Oversized/truncated input is an error. Servers process the requested number
of requests; a failed request terminates this introductory server.

**Current pattern:** `getaddrinfo`, explicit framing, complete stream I/O,
`SOCK_CLOEXEC`, and `MSG_NOSIGNAL`. Connection setup and each blocking socket
operation have three-second timeouts. These per-operation limits are not a
whole-request deadline against a peer that keeps sending occasional bytes.

**Fragile recipe:** assume one send equals one receive, assume UDP delivery,
or unlink a preexisting Unix socket path unconditionally. This server refuses
existing paths and removes only its own bound pathname on normal/error exit.
Use a private directory for real local services; forced termination may leave
the path for its owner to inspect and remove.

Exercise: fragment the header into individual writes, half-close the client
after its request, and compare TCP's stream with UDP's message boundaries.
