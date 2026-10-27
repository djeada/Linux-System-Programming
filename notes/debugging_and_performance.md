# Debugging and performance workbook

Companion: [lab16](../labs/lab16/README.md). Apply these observations to every
lab instead of treating debugging as an unrelated final topic.

## Form a question before collecting data

| Question | Observation |
|---|---|
| Which operation failed or blocks? | `strace` with relevant syscall filters and timestamps |
| Where is application state wrong? | GDB breakpoint, backtrace, variables, thread stacks |
| Which resources does this process own? | `/proc/PID/fd`, `maps`, `status`, `limits` |
| Is a memory access or lifetime invalid? | AddressSanitizer/UndefinedBehaviorSanitizer |
| Is shared access synchronized? | A separate ThreadSanitizer build, where supported |
| Where is CPU time spent? | Repeated workload plus `perf stat`/`perf record`, if permitted |

Tracing perturbs timing, optimized builds can inline/remove variables, and a
sanitizer passing one run does not establish correctness. Build with debug
symbols, state the workload, and keep normal and instrumented results distinct.

## Trace descriptor ownership

From the repository root:

```sh
make build/lab16
strace -f -tt -e trace=openat,read,write,close -o /tmp/lab16.trace \
  ./build/lab16 labs/lab16/main.c 10
```

Find each open of the input and its matching close. Compare this with the
program's before/after descriptor counts. The dynamic loader also opens files:
classify events by path and descriptor lifetime rather than counting all opens
as application leaks. For a shell pipeline, inspect which process retains a
write end when a reader never gets EOF. For lab12, trace `epoll_wait`,
`epoll_ctl`, `recvfrom`, `sendto`, and timer/signal reads. See the
[strace manual](https://man7.org/linux/man-pages/man1/strace.1.html).

## Stop at the application boundary

```sh
cc -std=c11 -O0 -g -Wall -Wextra -Wpedantic labs/lab16/main.c -pthread -o build/lab16_debug
gdb --args ./build/lab16_debug labs/lab16/main.c 2
```

Use `break checksum_bytes`, `run`, `bt`, `print size`, `next`, and `continue`.
While paused, inspect `/proc/PID/fd` and `/proc/PID/maps` from another terminal.
For threaded labs, `info threads` and `thread apply all bt` reveal which workers
are waiting and which object they wait on. Understand the predicate, not just
the top stack frame. See the [GDB manual](https://sourceware.org/gdb/current/onlinedocs/gdb/).

## Instrument memory and lifetimes

Compile a normal lab with `-O1 -g -fsanitize=address,undefined
-fno-omit-frame-pointer`. Run the same behavioral cases. Compile thread-race
checks separately with `-fsanitize=thread`; it cannot simply be combined with
AddressSanitizer. The opt-in leak antipattern intentionally fails so you can
read the allocation backtrace and add its missing `free`. See
[Clang AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html).

For the full regression suite:

```sh
make test
```

Tests use per-subprocess `LD_PRELOAD` fault injection to check failures that
are otherwise hard to reproduce. Sanitizer runtimes and preloaded fixtures
can conflict; run the scenarios without preload under sanitizers separately.

## Measure a repeatable workload

Run enough iterations to dominate startup; report input size, concurrency,
CPU/storage context, compiler flags, and elapsed/CPU time. `ru_maxrss` is a
Linux peak-RSS value in KiB, not live allocated heap. Page cache state and
instrumentation affect I/O results. Profiling restrictions are host policy:
report them and use available tools rather than changing sysctls. Optimize
only after locating a material bottleneck and preserving behavior.
