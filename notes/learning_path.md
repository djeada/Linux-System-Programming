# Learning path and coverage

The course now has twenty runnable lab topics, with the liburing lab optional.
The original synchronization exercises remain useful, while the additions
broaden practical Linux application programming. This is a foundation with
advanced introductions, not a claim to cover every Linux subsystem.

## Suggested sequence

| Stage | Labs | Outcome |
|---|---|---|
| Observe and own resources | [16](../labs/lab16/README.md), [9](../labs/lab9/README.md), [10](../labs/lab10/README.md) | Trace a program, handle complete I/O, understand names and persistence |
| Processes and signals | [1](../labs/lab1/README.md), [2](../labs/lab2/README.md), [3](../labs/lab3/README.md) | Handle input/signals, launch and reap processes |
| Shared work and synchronization | [4](../labs/lab4/README.md)–[8](../labs/lab8/README.md) | Coordinate processes/threads, bound queues, define shutdown |
| Network protocols and readiness | [11](../labs/lab11/README.md), [12](../labs/lab12/README.md) | Frame streams, compare datagrams, manage bounded event state |
| Terminal and service lifecycle | [13](../labs/lab13/README.md), [14](../labs/lab14/README.md) | Own jobs/terminals; report readiness, reload, and stop |
| Linux restriction mechanisms | [15](../labs/lab15/README.md) | Distinguish identities, privileges, namespaces, and resource limits |
| Advanced API introductions | [17](../labs/lab17/README.md)–[20](../labs/lab20/README.md) | Reason about ordering, ABI/address lifetime, and I/O completions |

Run `make test` after changes and use the diagnostics workbook throughout.
Each lab README documents its grammar, bounds, failure behavior, build command,
and exercises. Quizzes cover both OS models and implementation choices.

## Seven additions to the core syllabus

| Topic | Implementation | Notes |
|---|---|---|
| Descriptor I/O | Lab9: ordinary and positional copier | [File I/O](file_io_and_filesystems.md) |
| Filesystems/persistence | Lab10: traversal, metadata, links, replacement | [Filesystems](file_io_and_filesystems.md) |
| Networking/local IPC | Lab11: TCP, UDP, Unix streams | [Networking](networking_and_event_loops.md) |
| Event-driven I/O | Lab12: nonblocking epoll, timers, signals, bounded clients | [Event loops](networking_and_event_loops.md) |
| Process groups/terminals | Lab13: pipelines, redirection, jobs/bg/fg | [Terminal control](shells_and_terminals.md) |
| Service lifecycle | Lab14: readiness, reload, logs, graceful stop, optional user unit | [Services](service_lifecycle.md) |
| Security/isolation | Lab15: credentials, caps, limits, seccomp, user/UTS namespaces, delegated cgroups | [Isolation](security_and_isolation.md) |

The [diagnostics workbook](debugging_and_performance.md) accompanies these
topics. Advanced notes introduce [C11 atomics](atomics_and_memory_order.md),
[runtime loading/file mappings](dynamic_loading_and_mappings.md), and
[io_uring](asynchronous_io.md).

## Scope for later specialization

Potential follow-ons include Unix descriptor passing/peer credentials, TLS and
DNS behavior, inotify/fanotify, robust recovery after owner death, pidfds and
subreapers, namespace combinations/mount propagation, container networking,
eBPF, NUMA/affinity/real-time analysis, detailed filesystem recovery, and
multi-request io_uring cancellation. They require their own bounded examples
and validation. Kernel modules, drivers, and kernel internals are a separate
course from userspace Linux system programming.

An API's age is not its recommendation status. Keep `fork`, pthreads, and
semaphores where they fit; isolate historical models, intentionally incorrect
antipatterns, and low-level comparisons so the main implementation path stays
clear. See the [pattern guide](modern_c_patterns.md).
