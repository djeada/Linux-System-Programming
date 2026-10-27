# Lab 17: C11 atomics and the memory model

Study an independent shared counter before considering lock-free data
structures. Read [atomic ordering notes](../../notes/atomics_and_memory_order.md).

```sh
make build/lab17 comparisons
./build/lab17 4 100000
./build/atomic_publication
```

Each worker increments one atomic counter with `memory_order_relaxed`.
Main joins workers before reading their ordinary per-worker totals. The final
counter must equal the sum of completed iterations. Arguments are 1–256
workers and a nonnegative iteration count; overflow of the expected total is
rejected. Partial thread startup joins all started workers before cleanup.
`lock_free` reports this platform's property rather than assuming it.

The separate comparison publishes an ordinary payload using a release store
and an acquire load of `ready`. Its tiny sleep-polling loop isolates ordering
for study. Use condition variables or the bounded queue for ordinary waiting.

**Current pattern:** choose ordering from the data dependency. A standalone
count needs atomicity, while publishing other memory needs synchronization.
**Incorrect recipe:** use `volatile` for thread synchronization, make every
field atomic without a protocol, or read a non-atomic payload before acquire.

Exercise: explain which operation makes the payload visible; compare the
counter with a mutex implementation without assuming the atomic version is
faster. A mutex-protected queue remains the recommended starting point for
compound work ownership. Some platforms require an additional `-latomic` when
their atomic width is not directly supported; the reference build targets
ordinary 64-bit Linux.
