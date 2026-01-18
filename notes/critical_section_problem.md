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

This is the failure the critical section problem is designed to prevent.

## Requirements for a correct solution

- mutual exclusion: if one process is in its critical section, no other process is in theirs
- progress: if no one is in the critical section, the choice of who enters next cannot be delayed by unrelated work
- bounded waiting: once a process asks to enter, it will get a turn after a finite number of entries by others

## Example in plain words

Imagine a printer shared by two users. If both try to send a job at the same time without coordination, pages can interleave and the output becomes unreadable. A critical section rule is the policy that says only one job may use the printer at a time, and everyone else must wait their turn.

Solutions that enforce these rules include locks, semaphores, monitors, and disabling interrupts in very specific low-level cases. The details differ, but the goal is always the same: keep shared data consistent.
