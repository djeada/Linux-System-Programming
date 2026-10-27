# Lab 16: Debugging and observing a process

Repeat a checked file workload, calculate an observable checksum, inspect
descriptor ownership, and report CPU time/peak resident memory. See the
[diagnostics workbook](../../notes/debugging_and_performance.md).

```sh
make build/lab16
./build/lab16 labs/lab16/main.c 10
strace -f -e trace=openat,read,close ./build/lab16 labs/lab16/main.c 2
cc -std=c11 -O0 -g -Wall -Wextra -Wpedantic labs/lab16/main.c -pthread -o build/lab16_debug
gdb --args ./build/lab16_debug labs/lab16/main.c 2
```

In GDB use `break checksum_bytes`, `run`, `bt`, `print size`, and `continue`.
Compare `/proc/PID/fd`, `/proc/PID/maps`, and `getrusage` with the syscall trace.
The before/after descriptor counts exclude the counting directory descriptor
and must match. Input is read without modification. Linux `ru_maxrss` is in
KiB and is a peak, not the current heap size.

Build sanitizer variants with `-fsanitize=address,undefined -fno-omit-frame-pointer`.
The deliberately incorrect `antipatterns/leak.c` is excluded from `make all`:

```sh
make -B antipatterns CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Wpedantic -fsanitize=address'
ASAN_OPTIONS=detect_leaks=1 ./build/leak_antipattern
```

LeakSanitizer should report its missing `free` and exit with failure on supported
hosts. Use it to learn the diagnostic, then add the missing cleanup yourself.
Compare a tracing/debug build with a normal run before interpreting performance.
`perf` and tracing may be restricted by host policy; record the limitation and
use the available observations. The build never changes those policies.
