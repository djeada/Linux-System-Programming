# Operating Systems

An operating system is the layer that turns raw hardware into a usable machine. It schedules work, protects programs from each other, and offers the familiar abstractions we rely on every day: files, processes, and memory. Without it, every program would need to manage devices and hardware details on its own.

Here is the big picture:

App programs
     |
system calls
     |
    OS kernel
     |
   hardware

The OS lives in the middle. It listens to requests from programs, decides who gets what and when, and talks directly to the hardware on their behalf.

## Responsibilities

- Process management: create, schedule, and terminate processes so the CPU stays busy and programs get fair turns.
- Memory management: track used and free memory, isolate processes, and move data between RAM and storage when needed.
- Device and I/O management: coordinate access to disks, keyboards, networks, and other devices.
- File management: organize data into files and directories with naming, permissions, and persistence.
- Protection and security: enforce access control and prevent one program from crashing or spying on another.
- User interface: provide shells, GUIs, and system utilities that make the machine usable.

## Types of operating systems

### Batch operating systems

Batch systems group jobs and run them in sequence with little or no interaction. Think of payroll processing or printing large reports overnight. The goal is throughput, not responsiveness.

Batch queue (time runs left to right)
Job A | Job B | Job C | Job D

Example: early mainframes where users submitted jobs on punch cards and waited for printed output.

### Multiprogramming operating systems

Multiprogramming keeps multiple jobs in memory at once and switches the CPU when one job waits for I/O. This improves CPU utilization.

CPU: [Job A] [Job B] [Job A] [Job C]
I/O:     [disk]        [net]

Example: classic mainframe systems that interleaved CPU-bound and I/O-bound jobs to avoid idle time.

### Multitasking operating systems

Multitasking adds rapid context switching so it feels like many programs run at the same time. The system slices CPU time into small chunks and rotates among ready tasks.

Time slices
App1 | App2 | App3 | App1 | App2 | ...

Example: modern desktop systems where a browser, editor, and music player appear to run simultaneously.

### Time-sharing operating systems

Time-sharing is multitasking aimed at multiple users. The system stays interactive by giving each user a quick, regular slice of the CPU and responsive I/O.

Users on one system
U1 -> slice -> U2 -> slice -> U3 -> slice -> repeat

Example: multi-user UNIX systems accessed through terminals.

### Real-time operating systems

Real-time systems focus on deadlines. The right answer delivered late can be a failure, so the scheduler prioritizes predictable timing over raw throughput.

Hard real-time: missing a deadline is unacceptable
Soft real-time: occasional misses are tolerable

Example: industrial controllers, medical devices, automotive airbags, and robotics.

## Why this matters

Knowing what an OS does explains why programs behave the way they do. A slow file copy, a laggy UI, or a process that gets killed all trace back to OS policies. When you understand those policies, you can reason about performance, safety, and reliability with far more confidence.
