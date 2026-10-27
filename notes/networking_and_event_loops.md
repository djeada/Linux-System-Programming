# Networking and event-driven I/O

Companion labs: [11: sockets](../labs/lab11/README.md) and
[12: epoll](../labs/lab12/README.md).

## Choose the transport and protocol

| Transport | What the API preserves | Typical use in these labs |
|---|---|---|
| TCP | Ordered byte stream; no application message boundaries | Framed request/response over IP |
| UDP | Datagram boundaries; delivery/order/uniqueness not guaranteed | Small independent echoes |
| Unix stream | Local byte stream; pathname/peer access has local OS semantics | Framed local IPC |

Use `getaddrinfo` rather than assuming every host has one IPv4 address. Try
candidate addresses and close failed sockets. Lab11 supports IP candidates;
lab12 deliberately binds only IPv4 loopback to keep event-loop mechanics clear.
TCP reliability does not supply application deadlines, authentication, TLS,
or successful processing after a connection fails.

Both stream labs use a four-byte big-endian length followed by at most 64 KiB
of payload. A receiver accumulates the complete header, validates the length,
then accumulates the body. Zero length is valid. EOF in the middle is a
truncated frame. A read can split or combine application writes. Datagram
limits are separate; lab11 uses `MSG_TRUNC` to detect an oversized datagram.
See [tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html),
[udp(7)](https://man7.org/linux/man-pages/man7/udp.7.html), and
[unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html).

## Readiness is permission to try

With nonblocking sockets, readiness says an operation may make progress.
`EAGAIN` is a normal transition back to waiting, not a fatal application error.
Retain unfinished input/output in owned state. Handle `EINTR`, peer closure,
and reset/error events separately. `MSG_NOSIGNAL` makes a failed send return
an error without terminating the process through SIGPIPE.

Lab12 uses level-triggered `epoll`: unread input or writable capacity can keep
producing notifications. Edge-triggered mode changes the obligations; it
normally requires draining nonblocking operations until `EAGAIN`. It is not
automatically faster or simpler. Read
[epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html) before changing
the mode. `poll` remains a clear choice for a small descriptor set.

## Bound both memory and waiting

Lab12 bounds connections and each response buffer. When a response is pending,
it stops reading that peer, allowing pressure to propagate through socket
buffers. It registers writable interest only after a blocked send. Full
client tables temporarily disable accepting, leaving further connections to
the bounded kernel listen queue and client timeout policy.

Each accepted client has a monotonic last-progress time. Timer events expire
idle connections. Signals arrive through `signalfd`, so shutdown is a normal
event-loop transition: stop accepting, drain within a deadline, close remaining
peers. A half-close after a complete request still allows its response.
Generation tags keep queued events from referring to a new occupant of an
old client slot.

Per-operation socket timeouts in lab11 do not form an absolute request
deadline. A peer making occasional progress can extend a session. Production
protocols may need absolute frame deadlines, fair per-event work budgets,
authenticated admission, rate limits, and explicit cancellation. Start with
this bounded state machine before adding those policies.
