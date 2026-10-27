# Shared C helpers

These headers contain reusable functions and types, not runnable programs.
Compile a lab's `main.c`, or use `make` from the repository root. Keep this
directory alongside the labs when copying examples.

| Header | Responsibility |
|---|---|
| `system.h` | Checked parsing, clocks, pthread errors, owned child lifetimes |
| `process.h` | Process-shared semaphore initialization and operations |
| `bounded_queue.h` | Fixed-capacity thread queue with closure and deadlines |
| `shell.h` | Small whitespace-only launch loop for lab3 and its comparison |
| `io.h` | Interrupted/partial I/O, numeric ranges, file identity, synchronization |
| `net.h` | Bounded framing, socket setup, connection and I/O timeouts |

Include the appropriate helper **before system headers** so its feature-test
macros take effect. `io.h` enables documented GNU/Linux interfaces; the source
language remains C11. Check return values at the call site: these helpers do
not make ownership or application protocols automatic.
