# Scheduling Algorithms

CPU scheduling is about deciding who runs next when multiple processes are ready. Different algorithms trade off responsiveness, throughput, fairness, and predictability. There is no universal best choice, so operating systems pick policies that match their goals.

A simple mental model is a ready queue and a CPU:

ready queue -> scheduler -> CPU -> I/O or done

The algorithms below are **classical scheduling models** for comparing waiting
time, fairness, and preemption. They are not a promise that a current Linux
kernel uses these exact rules. For application scheduling, start with the
[fixed worker pool](../labs/lab7/README.md) and bounded queue; the kernel still
schedules those workers.

## First come, first served (FCFS)

FCFS runs processes in arrival order and never preempts.

Timeline example (burst times in parentheses)

P1(5) arrives, then P2(2), then P3(3)

CPU: P1 | P2 | P3

It is easy to implement but suffers from the convoy effect: one long job can delay many short ones.

## Shortest job first (SJF)

SJF chooses the process with the smallest next CPU burst. For the textbook case where all jobs are ready and burst lengths are known, this nonpreemptive order minimizes average waiting time.

CPU: P2(2) | P3(3) | P1(5)

The downside is starvation: long jobs can wait indefinitely if short ones keep arriving.

## Shortest remaining time first (SRTF)

SRTF is the preemptive version of SJF. If a new job arrives with a shorter remaining time, it preempts the running job.

Arrival example

Time 0: P1(8) starts
Time 2: P2(3) arrives -> P1 preempted
CPU: P1 | P2 | P1

It improves responsiveness for short jobs but adds context switch overhead.

## Round robin (RR)

Round robin gives each process a fixed time slice and cycles through the ready queue.

Quantum = 2
CPU: P1 | P2 | P3 | P1 | P2 | ...

Small quantum improves responsiveness but increases overhead. Large quantum behaves more like FCFS.

## Priority scheduling

Priority scheduling runs the highest priority process first. It can be preemptive or non-preemptive.

Priority level
high: P3
mid : P1
low : P2

CPU: P3 | P1 | P2

Starvation is a risk for low priority processes. Aging is a common fix: priorities slowly increase the longer a process waits.

## Highest response ratio next (HRRN)

HRRN balances shortness and waiting by using a response ratio.

Formula

```
response ratio = (waiting + burst) / burst
```

This favors short jobs but gradually boosts long-waiting jobs so they still run.

## Multilevel queue (MLQ)

MLQ splits processes into separate queues, often by type, and schedules queues by fixed priority.

Example queues

Q0 interactive
Q1 batch
Q2 background

CPU always picks from the highest non-empty queue. The drawback is rigidity: a process usually cannot move between queues.

## Multilevel feedback queue (MLFQ)

MLFQ allows processes to move between queues based on behavior. CPU-hungry jobs drift down, interactive jobs float up.

Queues from top to bottom
Q0 -> Q1 -> Q2

Typical rules

- new jobs start in Q0
- using a full time slice moves a job down
- blocking for I/O can keep a job at the same level or move it up

This adapts to workloads. Preventing starvation requires explicit rules such as periodic priority boosts; the multilevel structure alone does not guarantee progress for low-priority jobs.

## Choosing an algorithm

Use the models to identify tradeoffs, then measure the actual workload and
platform. In the labs, bounded queues control backlog, blocking waits let other
workers run, and monotonic clocks define elapsed-time cutoffs. The
[garage](../labs/lab6/README.md) implements an explicit phase-fair admission policy;
OS scheduling alone cannot enforce that application invariant. Real-time
requirements need their own scheduling and worst-case timing analysis.
