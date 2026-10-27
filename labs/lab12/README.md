# Lab 12: A bounded epoll event loop

Serve concurrent local TCP sessions without a thread per connection. Reuse
lab11's frame protocol. Read [the event-loop notes](../../notes/networking_and_event_loops.md).

```sh
make build/lab12 build/lab11
./build/lab12 8080 0 3000
# Another terminal:
./build/lab11 client tcp 127.0.0.1 8080 'event-driven hello'
```

Arguments are `port [max_sessions=0] [idle_ms=3000]`. Port zero selects an
ephemeral port. Zero sessions means run until SIGINT/SIGTERM. A positive limit
stops intake after that many accepted connections and drains them. Binding is
restricted to IPv4 loopback for this exercise.

The **level-triggered** loop owns nonblocking/CLOEXEC sockets, `epoll`, a
monotonic `timerfd`, and a `signalfd`. Each of 64 client slots holds at most
one 65,536-byte frame plus its header. Header/body/read/write progress lives
in that slot. Stop reading while its response is pending; watch `EPOLLOUT`
only when a send would block. Stop accepting when the table is full and
resume when slots free. Generation numbers reject stale slot notifications.

Half-closing a request does not discard its response. Bad frames and idle
peers close only their own sessions. Idle deadlines are checked every 50 ms.
SIGINT/SIGTERM stops intake and gives existing peers at most `idle_ms` to
finish; remaining descriptors are then closed.

**Fragile recipe:** switch to nonblocking I/O but spin on `EAGAIN`, register
writable interest permanently, or allow unlimited per-peer buffers. The
bounded buffer is the backpressure policy, with idle expiration for stalled
peers. This is an educational single-frame server, without TLS, authentication,
multi-frame streaming, or a production admission policy. Extend it only after
testing fragmented headers, slow peers, half-closes, and timeout cleanup.
