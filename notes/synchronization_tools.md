# Synchronization Tools

Concurrency is powerful but risky. When two threads touch shared data at the same time, tiny timing shifts can corrupt state. Synchronization tools are the OS and programming patterns that keep shared resources safe, consistent, and fair.

This note covers the most common tools and the deadlock risks that come with them.

## Semaphores

A semaphore is a protected counter with two atomic operations, often called wait (P) and signal (V). The count represents how many units of a resource are available.

Binary semaphore

Value is 0 or 1. It behaves like a lock.

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

Mutexes are ideal for protecting single shared structures like a queue, a list, or a file handle.

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

Condition variables are good for producer-consumer style coordination without busy waiting.

## Spinlocks

A spinlock is a lock that waits by looping, repeatedly checking the lock until it becomes free.

spin: while locked, keep spinning

Spinlocks are useful for very short critical sections on multicore CPUs, but they waste CPU time if held too long.

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

Fair scheduling policies and bounded waiting reduce starvation risk.

## Handling deadlocks

- prevention or avoidance: structure the system so one condition cannot happen
- detection and recovery: allow deadlocks, detect them, then recover
- ignore: if deadlocks are rare, some systems accept the risk

## Banker's algorithm

Banker's algorithm is a classic avoidance technique. It only grants a request if the system stays in a safe state, meaning all processes could still finish in some order.

Safe state idea

Available: 3
Needs: P1=2, P2=1, P3=3

Granting P2 is safe because P2 can finish, release resources, and then others can finish.
