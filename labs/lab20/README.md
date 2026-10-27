# Lab 20: Optional io_uring introduction

Learn submission entries, completion entries, negative errno results, and
buffer lifetime with liburing. Read [asynchronous I/O notes](../../notes/asynchronous_io.md).

On Debian/Ubuntu the optional development dependency is `liburing-dev`:

```sh
make optional
# If liburing development files are available:
./build/lab20 source_file destination_file
```

`make all` needs no liburing and excludes this lab. `make optional` prints a
clear skip if pkg-config cannot find it. Once installed, `make build/lab20`
builds it directly. Custom installations may supply `LIBURING_CFLAGS` and
`LIBURING_LIBS`; tests use `PKG_CONFIG_PATH` to discover that installation.

The copier sets up a ring **before opening/truncating the destination**,
rejects source/destination aliases, and submits explicit-offset reads/writes.
It checks submission and completion results separately, consumes each CQE,
handles short writes, and retains the buffer until completion. It syncs the
finished destination and tears down the ring on exit. A failed copy can leave
a partial destination; this is not the atomic replacement lab.

There is deliberately **one request in flight**. Waiting for each completion
blocks this caller; this is a lifecycle example, not a throughput claim or a
replacement for lab12's readiness model. Exercises: add multiple owned buffers
and request IDs, then reason about completion reordering and cancellation
before measuring pipelined performance.

Kernel/container policy may reject ring creation even when liburing is present.
Unsupported or denied setup prints `SKIP` and exits **77**, without touching the
destination. Do not change host kernel policy to hide that result. Tests report
missing dependencies or setup restrictions explicitly rather than treating
them as a successful I/O run.
