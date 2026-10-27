# Synchronization Tools

The [C11 atomic ordering notes](atomics_and_memory_order.md) extend this topic
with independent counters and publication. Start compound work ownership with
the mutex/condition queue; atomics require a separate correctness argument.

Concurrency is powerful but risky. When two threads touch shared data at the same time, tiny timing shifts can corrupt state. Synchronization tools are the OS and programming patterns that keep shared resources safe, consistent, and fair.

Use this note with the [modern C/POSIX guide](modern_c_patterns.md): the lab
implementations use blocking waits, bounded buffering, explicit closure, and
checked lifetimes. Conceptual semaphore `signal` below means posting a permit;
it is different from installing a C signal handler.

## Semaphores

A semaphore is a protected counter with two atomic operations, often called wait (P) and signal (V). The count represents how many units of a resource are available. A count above one allows concurrent access; a separate mutex is needed for a shared mutable invariant.

Binary semaphore

A binary-semaphore protocol keeps the value at 0 or 1. Unlike a mutex, a semaphore has no owning thread: another participant may post it. Balanced use is an application invariant.

Counting semaphore

Value is 0..N. It models N identical resources, such as a pool of database connections.

Conceptual flow

wait: if count > 0, decrement and enter
      else block
signal: increment and wake a waiter if needed

## Mutex

A mutex is a mutual exclusion lock. Only one thread can hold it at a time, so it protects a critical section.

Lock flow

thread A: lock -> critical section -> unlock
thread B: waits until unlock

A mutex protects an invariant followed by every conflicting access. Ordinary pthread mutexes are process-private; sharing one across processes requires shared backing memory and process-shared initialization. The process labs instead use POSIX semaphores initialized with `pshared = 1`.

## Condition variables

A condition variable lets threads sleep until a condition becomes true. It is always used with a mutex.

Typical pattern

lock(m)
while condition is false:
  wait(cond, m)
do work
unlock(m)

Another thread does:

lock(m)
update condition
signal(cond) or broadcast(cond)
unlock(m)

A notification does not store the condition: inspect the predicate while
holding the same mutex, including after spurious wakeups. The
[bounded queue](../labs/common/bounded_queue.h) has separate readable/writable
conditions. A full queue blocks its producer; closure wakes both kinds of waiter,
and consumers drain accepted items before exiting.

Elapsed-time waits use `CLOCK_MONOTONIC`, with the condition variable's clock
attribute set to the same clock. Reuse the absolute deadline after a wakeup.
A realtime clock adjustment must not silently extend an intake interval.

## Spinlocks

A spinlock is a lock that waits by looping, repeatedly checking the lock until it becomes free.

spin: while locked, keep spinning

Spinlocks are a low-level mechanism to study after blocking mutexes. Busy waiting can waste CPU and delay the lock holder; choose it only when measurements and progress assumptions justify it.

## Readers-writer locks

Some data is read often and written rarely. A readers-writer lock allows multiple readers or one writer.

R and W example

R1 enters
R2 enters
W waits
R1 leaves
R2 leaves
W enters

The policy matters. Favoring readers can starve writers, and favoring writers can block readers.

## Barriers

A barrier makes a group of threads wait until everyone reaches the same point.

Barrier idea

T1 -> wait
T2 -> wait
T3 -> wait
all arrive -> all proceed

Barriers are common in parallel loops and simulation steps.

## Example: producer and consumer

One thread produces items, another consumes them. A mutex protects the buffer, and semaphores track empty and full slots.

buffer slots: [ ][ ][ ]
empty = 3, full = 0

producer: wait(empty) -> lock -> add item -> unlock -> signal(full)
consumer: wait(full)  -> lock -> remove item -> unlock -> signal(empty)

This keeps the buffer consistent and prevents overfill or underflow.

## Deadlock

Deadlock happens when each process holds a resource and waits for another, forming a cycle. No one can move.

Cycle example

P1 holds R1, waits for R2
P2 holds R2, waits for R1

Four necessary conditions

- mutual exclusion: some resources are non-shareable
- hold and wait: a process holds one resource and waits for another
- no preemption: resources cannot be forcibly taken
- circular wait: there is a cycle of waiting

If all four hold at once, deadlock is possible.

## Livelock and starvation

Deadlock is not the only failure mode.

- livelock: threads keep reacting to each other but make no progress
- starvation: a thread waits indefinitely because others keep winning

Mutual exclusion alone does not guarantee FIFO service or bounded waiting.
[Lab6](../labs/lab6/README.md) reserves capacity before posting each vehicle's
gate and stops extending a type batch when the other type waits. Alternating
batches provides a deliberate admission policy, separate from OS scheduling.

## Handling deadlocks

- prevention or avoidance: structure the system so one condition cannot happen
- detection and recovery: allow deadlocks, detect them, then recover
- ignore: if deadlocks are rare, some systems accept the risk

## Banker's algorithm

**Classical model:** Banker's algorithm is an avoidance technique requiring known maximum resource demands. It is not the default lock-management algorithm used by these C programs. It only grants a request if the system stays in a safe state, meaning all processes could still finish in some order.

Safe state idea

Available: 3
Needs: P1=2, P2=1, P3=3

Granting P2 is safe because P2 can finish, release resources, and then others can finish.

## Completion is a separate predicate

An empty work queue does not imply every task is finished. A consumer may be
working on an item it has already removed. The restaurant closes intake, drains
the queue, joins all chefs, and only then asks its reporter for a final snapshot.
The barber uses a completion semaphore per customer so an unrelated customer
cannot consume another haircut's completion notification.
