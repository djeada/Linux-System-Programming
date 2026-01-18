# Disk Scheduling

Disk scheduling is about deciding the order of disk I/O requests so the read/write head does not waste time moving around. On spinning disks, head movement is slow compared to CPU speed, so good scheduling can make the whole system feel faster.

Think of a disk like a long hallway of tracks. Moving the head costs time, and the scheduler tries to minimize that travel.

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

10, 20, 35, 55, 70, 90, 150

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
50 -> 55 -> 70 -> 90 -> 150 -> reverse -> 35 -> 20 -> 10

Wait times are more predictable than FCFS.

### C-SCAN (circular SCAN)

Like SCAN, but only service in one direction. When the head reaches the end, it jumps back without servicing.

50 -> 55 -> 70 -> 90 -> 150 -> jump -> 10 -> 20 -> 35

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

The algorithms above were designed for spinning disks. SSDs have no moving head, so seek time and rotational latency are almost zero. On SSDs, the goals shift toward throughput, fairness, and write amplification.

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

## Modern I/O schedulers

Linux and other OSes offer multiple I/O schedulers tuned for different workloads.

- noop: minimal reordering, good for SSDs or hardware RAID
- deadline: limits how long requests can wait to reduce latency spikes
- cfq or bfq: fair sharing across processes and cgroups

Choosing the right scheduler depends on the storage device and whether you care more about latency or throughput.

## When each is used

FCFS is simple but rarely optimal. SSTF gives good throughput but can starve. SCAN and LOOK are balanced choices for general workloads. C-SCAN and C-LOOK aim for fairness by keeping wait times uniform across the disk.
