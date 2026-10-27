# C11 atomics and memory ordering

Companion: [lab17](../labs/lab17/README.md). C atomics are a language-level
topic; Linux syscall ordering does not replace the C memory model.

## Atomicity and publication are different requirements

A concurrent unsynchronized read/write of an ordinary object can be a data
race with undefined behavior. `volatile` does not make it atomic or establish
inter-thread ordering. An atomic object permits defined concurrent operations
on that object. Ordering of other objects requires a synchronization protocol.

| Operation in this course | Ordering and reason |
|---|---|
| Independent increment-only counter | Relaxed RMW: each increment is indivisible; no other data is published |
| Publish a completed ordinary payload | Release store of ready, acquire load that observes it |
| Read worker-owned ordinary results after completion | Successful pthread joins establish visibility |
| Compound queue ownership and waiting | Mutex/condition protocol, as in labs7/8 |

The publication comparison writes the payload, then release-stores true. Main
acquire-loads true before reading that payload. The release/acquire relationship
makes the earlier payload write visible; merely using relaxed on both sides
would not establish that relationship. Initializing the message before starting
the producer also has a clear lifetime/ownership boundary.

## Select a protocol before weakening ordering

Sequential consistency is the default for the simple C atomic APIs, and often
a useful starting point. Relaxed is sufficient for this particular independent
count. It is not a general performance switch to apply everywhere. An atomic
load of one field does not make a multi-field invariant consistent.

`atomic_is_lock_free` reports an implementation property for the object.
Atomic does not promise lock-free, wait-free, low contention, or better speed
than a mutex. Lab17 shares one counter, so cache-line contention is part of its
workload. Per-worker counts plus a final sum may be the better design when
intermediate global visibility is unnecessary.

The comparison's sleep polling keeps the ordering relation small enough to
study. For application work arrival, use blocking notification. Designing a
lock-free queue additionally requires ownership, memory reclamation, ABA
handling where relevant, and a progress argument; replacing integer fields
with atomics does not supply those.

Reference: the committee's [C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf),
particularly sections 5.1.2.4 and 7.17. Test data races with suitable tools, but
also explain the happens-before relationships in the code review.
