# The Critical Section Problem

When multiple processes or threads touch the same data at the same time, small timing differences can cause big problems. The critical section problem is the OS and concurrency answer to that danger: make sure shared data is accessed in a safe, predictable way.

Think about a shared bank balance. Two threads try to deposit $1 at the same time. If the operations overlap, one update can get lost.

## Key terms

- critical section: the part of code that reads or writes shared data
- remainder section: code that does not touch the shared data
- race condition: the final result depends on the timing of events rather than the logic

Typical structure of a process that uses shared data

do
  entry section
  critical section
  exit section
  remainder section
loop

The entry and exit sections are where synchronization happens. The critical section is where the shared data is actually touched.

## Why races happen

Even a simple increment is not one action. It is a read, a modify, and a write.

Shared counter starts at 5

Time ->
T1: read 5     modify -> 6     write 6
T2:     read 5     modify -> 6     write 6

Final value is 6, but two increments happened. One update vanished.

The diagram illustrates a lost update. In actual C11 threads, unsynchronized
conflicting non-atomic accesses constitute a data race and have undefined
behavior; this one outcome is not the full set of possible behavior. A `volatile`
flag does not provide thread synchronization. Use a mutex or suitable C11 atomic
operations and establish a complete protocol.

## Requirements for a correct solution

- mutual exclusion: if one process is in its critical section, no other process is in theirs
- progress: if no one is in the critical section, the choice of who enters next cannot be delayed by unrelated work
- bounded waiting: once a process asks to enter, it will get a turn after a finite number of entries by others

## Example in plain words

Imagine a printer shared by two users. If both try to send a job at the same time without coordination, pages can interleave and the output becomes unreadable. A critical section rule is the policy that says only one job may use the printer at a time, and everyone else must wait their turn.

For userspace C, start with a mutex around the complete state transition and
blocking conditions around wait predicates. The [bounded queue](../labs/common/bounded_queue.h)
updates its data, indices, and count under one mutex. Mutex ownership alone does
not guarantee fairness or bounded waiting; that requires an admission policy.

**Kernel mechanism:** disabling interrupts belongs to privileged kernel code
and is not an application-level substitute for a userspace lock. Classical
critical-section algorithms explain requirements; the
[implementation guide](modern_c_patterns.md) identifies the C/POSIX patterns
used in the executable labs.
