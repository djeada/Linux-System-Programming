# Processes and Threads

Continue the process-creation examples with [terminal job control](shells_and_terminals.md)
and [managed service lifecycle](service_lifecycle.md). They give processes an
explicit group, terminal, readiness, and shutdown protocol.

Processes and threads are the core units of work in an operating system. A process is a running program with its own private world. A thread is a smaller execution path inside that world. Knowing the difference explains why a browser can keep rendering while a download runs, and why a crash in one app does not take down the rest of the system.

## Process

A process is a program in execution. It has its own address space, execution context, and resources. The OS tracks each process so it can pause, resume, and protect it.

Typical process address space

high addresses
+------------------------+
| stack                  |
| (grows down)           |
+------------------------+
| heap                   |
| (grows up)             |
+------------------------+
| data and bss           |
+------------------------+
| code or text           |
low addresses

Process state flow

new -> ready -> running -> waiting -> ready -> terminated

## Process control block (PCB)

The PCB is the OS record for a process. It is the thing the scheduler actually moves around.

- process id and state
- program counter and register values
- memory pointers and page tables
- accounting info like CPU time
- I/O status and open files

## Context switching

A context switch is the moment the CPU stops running one process or thread and starts another. The OS saves the current CPU state into the PCB (or thread control block), then loads the next one.

Context switch sketch

CPU running P1
save P1 regs -> PCB1
load P2 regs <- PCB2
CPU running P2

Switches are necessary but not free. They cost time and can flush caches, so schedulers try to avoid excessive switching.

## Scheduling basics

Scheduling decides which ready process gets the CPU next. Most policies measure or compute a few common times:

- arrival time: when the process enters the ready queue
- burst time: how long it needs on the CPU
- completion time: when it finishes
- turnaround time: completion time minus arrival time
- waiting time: turnaround time minus burst time

Formulas

```
turnaround = completion - arrival
waiting = turnaround - burst
```

Example

Process A arrives at time 0, bursts for 5, completes at time 8
Process B arrives at time 1, bursts for 3, completes at time 4

Turnaround(A) = 8 - 0 = 8, waiting(A) = 8 - 5 = 3
Turnaround(B) = 4 - 1 = 3, waiting(B) = 3 - 3 = 0

This is why short jobs often feel faster, even if they start later.

## Thread

A thread is a lightweight execution path inside a process. Threads share the same address space but keep their own execution context.

Shared vs private

process
  code + data + heap + files
  |          |          |
  thread 1   thread 2   thread 3
  pc/regs    pc/regs    pc/regs
  stack      stack      stack

Threads are used for responsiveness, throughput, and clean separation of tasks. For reusable background work, start with a fixed worker pool and bounded queue, as in [lab7](../labs/lab7/README.md). For many mostly waiting connections, consider an event loop or bounded workers rather than an unlimited thread per connection.

## Process vs thread overhead

Processes are heavier because they have separate address spaces and more kernel bookkeeping. Threads are lighter because they share memory inside one process.

Typical tradeoffs

- processes: stronger isolation, higher creation and switch cost
- threads: cheaper creation, easy sharing, but bugs can corrupt shared state

## Creating processes and threads

For a direct external-program launch, [lab3](../labs/lab3/README.md) uses
`posix_spawnp`, then checks the child status with `waitpid`. Its
[fork/exec comparison](../labs/lab3/comparisons/fork_exec.c) makes custom child
setup explicit. Both APIs remain current; choose according to the work.

The forked-process labs have a single-threaded parent. A fork in a multithreaded
process imposes async-signal-safety restrictions on child operations before an
exec. Check failed forks, retry interrupted waits, and reap successfully started
children before releasing shared mappings. Child branches use `_exit` after
flushing intended output, so they do not run inherited parent cleanup.

If **every existing process** executes each of n successful fork calls, the total becomes 2^n. A parent-only loop that starts n children instead creates n + 1 processes; neither formula describes failed forks.

Threads are created with `pthread_create` and joined with `pthread_join`.
Arguments and shared state must outlive their workers. Pthread functions return
an error number directly, rather than setting `errno`. Track the number actually
started; on a partial startup failure, close work intake and join those workers.
[Lab8](../labs/lab8/README.md) starts consumers before producers, uses a monotonic
intake deadline, then joins every chef before reporting final completion.

## Types of threads

- user threads: managed by user libraries, the kernel may see only one process
- kernel threads: managed and scheduled by the OS kernel

Real systems often use a mix so the kernel can schedule threads while a user library manages policies.

## Threading models

There are a few ways user threads map to kernel threads.

1:1 model
user thread -> kernel thread

N:1 model
many user threads -> one kernel thread

M:N model
many user threads -> many kernel threads

Linux POSIX threads use a 1:1 mapping to kernel-scheduled threads and can run in parallel on available cores. The N:1 and M:N diagrams are comparative threading models, not implementation instructions for these labs.

## Inter-process communication (IPC)

Processes need ways to exchange data safely.

Common IPC tools

- pipes: byte streams; close unused endpoints so EOF can propagate, and handle complete logical records
- sockets: bidirectional communication across processes or machines
- shared memory: avoids explicit message copies but still needs process-shared synchronization and ownership
- message queues: structured messages with OS-managed buffering

IPC picture

Process A -> pipe -> Process B
Process A <-> socket <-> Process B

## Multilevel feedback queue (MLFQ) scheduling

MLFQ adapts to behavior. Short, interactive jobs stay near the top and get quick service. CPU-heavy jobs drift downward.

Priority queues

Q0 highest -> Q1 -> Q2 lowest

Typical behavior

- a job starts in Q0 with a short time slice
- if it uses the whole slice, it moves down
- if it blocks for I/O, it may stay or move up

Example idea

interactive shell: short bursts, stays near Q0
video encoder: long bursts, drifts toward Q2

The result is better responsiveness for interactive tasks without starving long ones.

## Ownership in the lab implementations

The [pattern guide](modern_c_patterns.md) separates current C/POSIX choices from
API comparisons and OS theory. In [lab2](../labs/lab2/README.md), each process opens
its input file after fork, obtaining independent file offsets. An inherited file
descriptor instead refers to the same open-file description, whose offset can be
shared by the parent and child. In [lab5](../labs/lab5/README.md), explicit buffer
completion ends the stream; a failure in one process stops and reaps its peers.
