# Memory Management

Memory management is the OS job of deciding what lives in RAM, where it lives, and for how long. It is part librarian, part traffic cop: keep the shelves organized, keep the CPU fed, and make sure programs do not stomp on each other.

This note walks through the main ideas, from early partitioning to paging and page replacement.

## Memory sharing techniques

Overlays

Only the parts of a large program that are needed right now are loaded into memory. This was common when RAM was small.

Program view
module A -> module B -> module C

Memory at one moment
+------------------+
| OS               |
| module B         |
| free             |
+------------------+

Swapping

When RAM is full, inactive processes can be moved to disk to free space. Later they are swapped back in.

RAM            Disk
[P1][P2][P3]   [P4][P5]
   swap out P2 ---->

Swapping keeps many processes in the system, but it is slow because disks are slow.

## Memory allocation schemes

Single partition

Memory is split into two regions: one for the OS and one for a single user process.

| OS | user process | free |

Multiple partitions

Memory is split into many partitions. These can be fixed-size or variable-size.

Fixed partitions

| OS | P1 | P2 | P3 | free |

This is simple but can waste space inside a partition if a process is smaller than the partition size. This is internal fragmentation.

Variable partitions

| OS | P1 | hole | P2 | hole | P3 |

Here each process gets a block close to its size. Over time, holes appear between blocks. This is external fragmentation.

Common placement strategies

- first fit: choose the first hole that is large enough
- best fit: choose the smallest hole that is large enough
- worst fit: choose the largest hole and leave a big remainder

Compaction can reduce external fragmentation by sliding processes together, but it is expensive.

## Paging

Paging splits memory into fixed-size frames and splits a process into fixed-size pages. Pages can be placed into any free frame, so a process no longer needs one contiguous block.

Logical pages -> physical frames

page 0 -> frame 5
page 1 -> frame 2
page 2 -> frame 9

Address translation uses a page table:

virtual address = (page, offset)
physical address = (frame, offset)

Paging removes most external fragmentation but can leave a little internal fragmentation inside the last page.

## Page tables and the TLB

Each process has a page table that maps virtual pages to physical frames. Page table entries usually store more than the frame number.

- present bit: is the page currently in RAM
- dirty bit: has the page been modified
- referenced bit: has the page been used recently
- protection bits: read, write, execute permissions

Looking up the page table for every memory access is slow, so CPUs cache recent translations in a small, fast table called the TLB.

Address translation with a TLB

CPU -> TLB hit -> frame -> RAM
CPU -> TLB miss -> page table -> TLB update -> RAM

Example address split

Page size 4096 bytes
Virtual address 12345
page = 3, offset = 57

The page table gives the frame number for page 3, and the offset stays the same.

## Segmentation

Segmentation divides a program by meaning: code, data, stack, heap. Each segment has its own size and grows independently.

Segment table

code  -> base 1000, limit 4k
data  -> base 9000, limit 2k
stack -> base 20000, limit 8k

Segmentation matches how programmers think about memory, but it can still suffer from external fragmentation. Many systems combine segmentation and paging.

## Page faults

A page fault happens when a process references a page that is not in RAM. The OS must fetch it from disk, update the page table, and restart the instruction.

Page fault flow

1) CPU references page X
2) page table says not present
3) OS loads page X into a free frame
4) page table updated, instruction retried

Page faults are normal in a virtual memory system, but too many cause thrashing.

## Thrashing and working sets

If the system spends more time handling page faults than running useful work, it is thrashing. The CPU sits idle while the disk churns.

CPU timeline
run | fault | wait | fault | wait | run

Working set is the set of pages a process is actively using over a short window of time. If the working set fits in RAM, page faults drop. If it does not, the process keeps evicting pages it will need again soon.

One common fix is to reduce the number of active processes so each gets enough frames.

## Page replacement algorithms

When RAM is full, the OS must pick a page to evict.

FIFO

Evict the page that has been in memory the longest. Simple, but it can suffer from Belady's anomaly where more frames cause more faults.

Example with 3 frames
refs: 1 2 3 4 1 2
frames after 1 2 3: [1 2 3]
next 4 evicts 1 -> [4 2 3]

Optimal

Evict the page that will not be used for the longest time in the future. It is not implementable in real systems but is a useful benchmark.

LRU

Evict the page that has not been used for the longest time in the past. This approximates optimal behavior and is common in practice.

LRU idea
recently used -> keep
long time ago -> evict

No replacement policy is perfect. The right choice depends on workload, hardware support, and performance goals.
