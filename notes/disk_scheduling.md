# Disk Scheduling

Disk scheduling is about deciding the order of disk I/O requests so the read/write head does not waste time moving around. On spinning disks, head movement is slow compared to CPU speed, so good scheduling can make the whole system feel faster.

**Classical HDD model:** think of a disk like a long hallway of tracks. Moving the head costs time, and the scheduler tries to minimize that travel.

Tracks: 0 ------------------------------------------ 199

## Disk access parameters

- seek time: time to move the head to the target track
- rotational latency: time for the right sector to spin under the head
- transfer time: time to actually read or write the data
- access time: seek + rotation + transfer
- response time: how long a request waits in the queue

Formula

```
access time = seek + rotation + transfer
```

## Scheduling algorithms

To compare algorithms, imagine a head starting at track 50 with requests at:

10, 90, 20, 150, 35, 70, 55

### First come, first served (FCFS)

Serve requests in arrival order. Simple and fair, but can cause long head movement.

Head path (arrival order)
50 -> 10 -> 90 -> 20 -> 150 -> 35 -> 70 -> 55

Good for fairness, not great for performance.

### Shortest seek time first (SSTF)

Always go to the closest request from the current head position.

Head path (one possible)
50 -> 55 -> 70 -> 90 -> 35 -> 20 -> 10 -> 150

This reduces total movement but can starve far requests if new nearby requests keep arriving.

### SCAN (elevator)

Move in one direction servicing requests, then reverse at the end.

If moving upward:
50 -> 55 -> 70 -> 90 -> 150 -> 199 -> reverse -> 35 -> 20 -> 10

Wait times are more predictable than FCFS.

### C-SCAN (circular SCAN)

Like SCAN, but only service in one direction. When the head reaches the end, it jumps back without servicing.

50 -> 55 -> 70 -> 90 -> 150 -> 199 -> jump to 0 -> 10 -> 20 -> 35

This gives a more uniform wait time across tracks.

### LOOK

LOOK is SCAN but it only goes as far as the last request in each direction, not the physical end of disk.

50 -> 55 -> 70 -> 90 -> 150 -> reverse -> 35 -> 20 -> 10

Saves travel when the queue does not reach the extreme ends.

### C-LOOK

C-LOOK is the circular version of LOOK. It goes to the last request in one direction, then jumps to the first request on the other side.

50 -> 55 -> 70 -> 90 -> 150 -> jump -> 10 -> 20 -> 35

This reduces unnecessary movement while keeping wait times fairly even.

## HDD vs SSD

The algorithms above were designed for spinning disks. SSDs have no moving head, so mechanical seek and rotational latency do not apply. On SSDs, the goals shift toward throughput, fairness, and write amplification.

HDD costs
seek + rotation dominate

SSD costs
controller queues and flash erase blocks dominate

## Request merging and reordering

Schedulers also try to merge nearby requests to reduce overhead.

Adjacent requests
[block 100] [block 101] [block 102]
merge -> [block 100..102]

This is useful on both HDDs and SSDs because it reduces per-request overhead.

## Current Linux I/O: inspect the actual device

Linux's block multi-queue (`blk-mq`) mechanism supports parallel software and
hardware queues. Device type, driver, kernel configuration, and workload matter;
a seek-minimizing HDD diagram is not an SSD/NVMe tuning prescription. The older
single-queue names `noop`, `deadline`, and `cfq` belong in historical comparisons,
not a universal list of current scheduler choices.

Read available policies from the device instead of assuming one is installed:

```sh
lsblk -d -o NAME,ROTA,TYPE,MODEL
# Example device; replace nvme0n1 with the name shown by lsblk.
cat /sys/block/nvme0n1/queue/scheduler
```

The bracketed entry is the selected scheduler. Compare throughput, latency, and
fairness for representative workloads before changing a policy. See the
[Linux blk-mq documentation](https://docs.kernel.org/block/blk-mq.html).

## Use the right model

FCFS, SSTF, SCAN, and LOOK explain mechanical head-travel tradeoffs. C-SCAN and
C-LOOK illustrate different service orders. Application-level work in these
labs instead uses bounded queues, backpressure, and checked I/O; see the
[modern C/POSIX guide](modern_c_patterns.md).
