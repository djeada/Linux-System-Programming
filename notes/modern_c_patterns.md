# Modern C/POSIX patterns and comparison examples

The application examples use **C11 on Linux**, with POSIX.1-2008 interfaces and
Linux shared mappings. C++ facilities such as `std::jthread` do not apply to
these C labs. Modern practice here means explicit ownership, checked operations,
bounded work, clear completion, and synchronization that matches the problem.
An older API is not automatically obsolete: `fork`, pthreads, and process-shared
semaphores remain useful when their semantics fit the work.

## Choose the pattern by the problem

| Problem | Starting point | Executable example |
|---|---|---|
| React to signals while waiting for input | `sigaction`, minimal handlers, self-pipe, and `poll` | [Maze navigator](../labs/lab1/main.c) |
| Read a file independently in two processes | Open after `fork`, read complete lines, and reap the child | [File reader](../labs/lab2/main.c) |
| Launch another executable | `posix_spawnp`, checked return code, and checked child status | [Shell](../labs/lab3/main.c) |
| Study explicit fork/exec setup | A separate API comparison using the same parser | [Fork/exec shell](../labs/lab3/comparisons/fork_exec.c) |
| Associate service completion with one customer | Shared FIFO and one completion semaphore per customer | [Sleeping barber](../labs/lab4/main.c) |
| Transform a stream between processes | Bounded shared buffers, blocking waits, explicit completion | [Text pipeline](../labs/lab5/main.c) |
| Admit compatible groups to limited capacity | Reserve slots under the mutex before waking selected waiters | [Parking garage](../labs/lab6/main.c) |
| Reuse worker threads | Fixed workers and a bounded, explicitly closed queue | [Thread pool](../labs/lab7/main.c) |
| Stop producing at a deadline and finish accepted work | Monotonic deadlines, queue closure, joins, then final reporting | [Restaurant](../labs/lab8/main.c) |
| Copy binary data or a file range | Complete I/O loops, checked file identity, `pread`/`pwrite` offsets | [Descriptor I/O](../labs/lab9/main.c) |
| Traverse/update a filesystem | Directory-relative opens; exclusive temporary file, sync, rename, directory sync | [Filesystems](../labs/lab10/main.c) |
| Exchange messages over sockets | Explicit stream framing or bounded datagrams, timeouts, owned endpoints | [Sockets](../labs/lab11/main.c) |
| Serve concurrent readiness events | Nonblocking level-triggered epoll, bounded client state, backpressure | [Event loop](../labs/lab12/main.c) |
| Launch terminal pipelines and jobs | Custom fork/exec setup, process groups, terminal ownership, complete reaping | [Job-control shell](../labs/lab13/main.c) |
| Run a managed service | Foreground loop, readiness notification, validated reload, bounded shutdown | [Service](../labs/lab14/main.c) |
| Study Linux isolation mechanisms | Inspect first, restrict owned children, require cgroup delegation | [Isolation](../labs/lab15/main.c) |
| Diagnose leaks or blocked operations | Observable workload, `/proc`, tracing/debugging/sanitizer exercises | [Diagnostics](../labs/lab16/main.c) |
| Count independent concurrent events | C11 atomic RMW; justify relaxed ordering and join owned threads | [Atomics](../labs/lab17/main.c) |
| Load trusted runtime extensions | Versioned C descriptor and library-owned pointer lifetime | [Plugin](../labs/lab18/main.c) |
| Modify file-backed memory | Validate bounds, choose shared/private semantics, coordinate access | [Mappings](../labs/lab19/main.c) |
| Study completion-driven file I/O | Optional liburing, retained buffer until checked completion | [io_uring](../labs/lab20/main.c) |

## Recommended patterns and older recipes

| Older or fragile recipe | Current application pattern | Reason |
|---|---|---|
| `signal()` handlers that print, allocate, or change application structures | `sigaction` handler that only notifies the main loop | Keep normal application work out of asynchronous handlers |
| `alarm` plus blocking buffered input for an interactive timeout | `poll` over input and a wake-up descriptor | Wait for input, a timeout, or notification in one loop |
| Always write a fork/exec child branch for program launching | Start with `posix_spawnp`; retain explicit fork/exec for custom setup | Make the launch operation and its ownership clear |
| Hardcode an input path and silently split fixed-size lines | CLI paths and `getline`, followed by field validation | Accept complete records and report invalid input |
| Allocate another queue node for every incoming task | Fixed-capacity queue with producer backpressure | Bound memory while work is arriving |
| Treat an empty queue as completed work | Close intake, drain, join workers, and report final state | A worker may still be processing a dequeued item |
| Count available slots only after a waiter wakes | Reserve capacity while selecting the waiter | Prevent new arrivals from using the same slot |
| Return on the first startup failure | Track successful initialization and started workers, then unwind | Leave no blocked peer or live worker using destroyed state |
| `perror` after a pthread function fails | Print the returned error number with `strerror` | Pthread errors are returned directly |
| Assume a write/send consumes a whole buffer | Advance by the returned byte count and preserve unfinished state | Short operations are valid |
| Assume a TCP read is one message | Length-bounded framing over the byte stream | Transport write boundaries are not preserved |
| Accept clients and allocate buffers without limits | Bound sessions and per-session storage; pause intake/reads | Overload needs an explicit policy |
| Sync the new file and assume its renamed name is durable | Sync the file, rename within its directory, sync the directory | Data and directory entries have separate persistence boundaries |
| Add `volatile` to a shared flag | Use an atomic ordering protocol or mutex/condition | Volatile does not establish inter-thread synchronization |
| Double-fork every service | Stay foreground under a manager and report readiness | Match the actual launch environment |

`posix_spawnp` covers direct launches and configurable file actions; it does not
replace every use of fork/exec. The process labs run child C work from a
single-threaded parent. For a fork in a multithreaded process, child operations
before exec have stricter safety requirements. See the
[process-creation manual](https://man7.org/linux/man-pages/man3/posix_spawn.3.html).

## Ownership and shutdown in C

The helpers in [system.h](../labs/common/system.h) make repeated rules visible:
flush intended output before a fork, track successfully started children,
retry interrupted waits, inspect statuses, and reap before freeing shared state.
The process labs supervise whichever child exits first so a failed stage can
stop peers blocked on IPC. They are standalone programs that own all their
children; the supervisor is not a general embedded process-management library.

[process.h](../labs/common/process.h) initializes process-shared semaphores with
rollback on partial failure. Each process lab keeps mappings and semaphores
alive until every child has finished. `_exit` avoids inherited parent exit
handlers in child branches; intended output is flushed explicitly first.

[bounded_queue.h](../labs/common/bounded_queue.h) is a C ring buffer with a
mutex and separate conditions for readable data and writable capacity. A push
waits for room, a pop waits for work, and closure wakes both kinds of waiter.
Accepted items are drained after closure. Initialize before starting threads;
join all users before destroying the queue. No per-task allocation is needed.

The restaurant starts consumers before producers. A startup failure closes the
queue before joining any producer that might be waiting for room. Normal
shutdown joins producers, closes the queue, joins chefs, then wakes the final
reporter. Runtime error handling and orderly worker shutdown are separate from
recovery after arbitrary crashes or forced termination.

## Signals and time

The maze handler updates `sig_atomic_t` flags and writes a byte to a nonblocking
self-pipe. The main loop prints and modifies the maze. Signal masking alone
does not make stdio safe inside a handler; see
[signal-safety(7)](https://man7.org/linux/man-pages/man7/signal-safety.7.html).

Elapsed-time deadlines use `CLOCK_MONOTONIC`. A condition variable used for those
deadlines must be initialized with the matching clock attribute. Spurious
wakeups recheck the predicate against the same absolute deadline. See
[condition-variable clock selection](https://man7.org/linux/man-pages/man3/pthread_condattr_getclock.3p.html).

`LAB_DELAY_MS` optionally changes simulated work delays. It does not change the
maze's five-second input timeout, reporting interval, or simulation deadline:

```sh
LAB_DELAY_MS=10 ./build/lab6 3 10 labs/lab6/vehicles.txt
```

## Theory and historical models

Overlays, contiguous partitioning, textbook page-replacement policies, and
spinning-disk seek algorithms explain OS design. They are not instructions for
implementing application memory management or selecting an SSD scheduler.
Likewise, disabling interrupts belongs to privileged kernel mechanisms rather
than ordinary userspace critical sections. The topic notes distinguish these
models from the C/POSIX patterns used in the labs.

## Build and verify

From the repository root:

```sh
make all
make comparisons
make test
```

Each lab README has its individual commands. Keep `labs/common/` with the
sources; those headers have no main function. Tests build labs1–19, the plugin,
and comparisons with strict C11 warnings treated as errors. Lab20 is built/tested
when pkg-config finds liburing. Checks include original concurrency behavior,
partial I/O, replacement failure boundaries, framed sockets, epoll admission and
timeouts, terminal job control, service reload, restrictions on disposable
children, plugin errors, and mapping persistence. Linux fault injection affects
only test subprocesses and needs no root access. Optional host restrictions and
missing delegated cgroups are recorded as skips.

The [learning path](learning_path.md) connects the new notes and exercises. Each
lab is deliberately scoped: an introductory deny filter is not a complete
sandbox, and a one-request io_uring example is not a performance recommendation.
