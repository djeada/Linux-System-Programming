# Linux System Programming

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))

A hands-on repository for learning Linux system programming through practical labs, notes, and quizzes. Twenty labs cover processes, threads, file I/O, filesystems, networking, event loops, terminal job control, services, isolation, diagnostics, and advanced interfaces. Reference implementations use C11 with current POSIX/Linux patterns. [The implementation guide](notes/modern_c_patterns.md) separates recommended application patterns, API comparisons, and historical OS models; the [learning path](notes/learning_path.md) connects the topics.

![linux_system_programming](https://github.com/user-attachments/assets/f3e18e63-7e2c-4705-b400-37fa533f6d35)

## Table of Contents

- [About](#about)
- [Repository Layout](#repository-layout)
- [Labs Overview](#labs-overview)
- [Notes](#notes)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Quick Start](#quick-start)
  - [General Build Pattern](#general-build-pattern)
- [Conventions](#conventions)
- [Troubleshooting](#troubleshooting)
- [Resources](#resources)
- [Contributing](#contributing)
- [License](#license)

## About

This repository is designed for students and developers who want to gain practical experience with Linux system programming. It covers essential topics including process management, inter-process communication, threading, synchronization mechanisms, and more. Each lab is a separate executable with detailed instructions and shared checked helpers; the notes provide the OS background for the exercises.

## Repository Layout

- `labs/` - programming assignments with `main.c` and lab-specific `README.md`
- `labs/common/` - shared error handling, process cleanup, parsing, and bounded queues
- `notes/` - course notes, the modern C/POSIX guide, and clearly labeled OS models
- `tests/` - strict builds, behavioral checks, and Linux fault-injection fixtures
- `quizzes/` - quick checks and practice questions
- `LICENSE` - MIT license

## Labs Overview

Each lab includes a dedicated README with objectives, constraints, and build/run instructions.

| Lab | Topic | Core Concepts | Difficulty | Est. Time |
| --- | --- | --- | --- | --- |
| [lab1](labs/lab1) | Maze Navigator | `sigaction`, self-pipe, `poll`, complete input | Beginner | 2-3 hours |
| [lab2](labs/lab2) | Forked File Reader | CLI paths, independent file I/O, child reaping | Beginner | 1-2 hours |
| [lab3](labs/lab3) | Mini Shell | `posix_spawnp`, complete lines, checked child status; fork/exec comparison | Intermediate | 3-4 hours |
| [lab4](labs/lab4) | Sleeping Barber | shared FIFO, per-customer completion, supervised processes | Advanced | 4-6 hours |
| [lab5](labs/lab5) | Text Pipeline | bounded shared buffers, stream preservation, failure propagation | Advanced | 4-5 hours |
| [lab6](labs/lab6) | Parking Garage | reserved capacity, per-vehicle gates, phase fairness | Advanced | 4-6 hours |
| [lab7](labs/lab7) | Thread Pool | reusable workers, bounded queue, explicit closure | Intermediate | 3-4 hours |
| [lab8](labs/lab8) | Restaurant Simulation | streamed orders, backpressure, monotonic deadlines, final reporting | Advanced | 4-5 hours |
| [lab9](labs/lab9) | Descriptor I/O | complete binary copies, partial I/O, `EINTR`, positional offsets | Intermediate | 2-3 hours |
| [lab10](labs/lab10) | Filesystems and Persistence | descriptor-relative traversal, metadata, links, durable replacement | Intermediate | 3-4 hours |
| [lab11](labs/lab11) | Network and Local Sockets | TCP/UDP/Unix sockets, framing, timeouts, half-closes | Intermediate | 3-4 hours |
| [lab12](labs/lab12) | Event-driven Server | nonblocking sockets, `epoll`, bounded buffers, timer/signal descriptors | Advanced | 4-6 hours |
| [lab13](labs/lab13) | Shell and Terminal Jobs | pipelines, redirection, process groups, `jobs`/`bg`/`fg` | Advanced | 5-7 hours |
| [lab14](labs/lab14) | Service Lifecycle | validated reload, graceful stop, logging, systemd readiness | Intermediate | 3-4 hours |
| [lab15](labs/lab15) | Security and Isolation | credentials, capabilities, limits, seccomp, namespaces, delegated cgroups | Advanced | 4-6 hours |
| [lab16](labs/lab16) | Diagnostics | `strace`, GDB, sanitizers, `/proc`, descriptor accounting | Intermediate | 2-4 hours |
| [lab17](labs/lab17) | C11 Atomics | relaxed counting, acquire/release publication, lifetime | Advanced | 3-4 hours |
| [lab18](labs/lab18) | Runtime Loading | shared libraries, versioned C ABI, `dlopen`/`dlsym`, unload ownership | Intermediate | 2-3 hours |
| [lab19](labs/lab19) | File-backed Mappings | private/shared mapping, advisory locks, `msync`, `mprotect` | Advanced | 3-4 hours |
| [lab20](labs/lab20) | Optional io_uring | submission/completion, buffer lifetime, explicit offsets; liburing required | Advanced | 3-5 hours |

The times are study estimates, including exercises. Follow the
[suggested learning order and coverage map](notes/learning_path.md) rather than
assuming lab numbers are the only prerequisite order. Lab15 reports restricted
optional mechanisms explicitly; lab20 is an optional dependency target.

## Notes

Start with **[Modern C/POSIX Patterns](notes/modern_c_patterns.md)** to choose an implementation approach and compare it with older recipes.

The `notes/` directory contains comprehensive course materials covering theoretical concepts that complement the hands-on labs:

- **[Operating Systems](notes/operating_systems.md)** - Overview of OS types, responsibilities, and fundamental concepts
- **[Processes and Threads](notes/processes_and_threads.md)** - Process lifecycle, scheduling, and thread management
- **[Memory Management](notes/memory_management.md)** - Virtual memory, paging, segmentation, and memory allocation strategies
- **[Synchronization Tools](notes/synchronization_tools.md)** - Mutexes, semaphores, and other synchronization primitives
- **[Critical Section Problem](notes/critical_section_problem.md)** - Race conditions, mutual exclusion, and solutions
- **[Scheduling Algorithms](notes/scheduling_algorithms.md)** - CPU scheduling policies and algorithms
- **[Disk Scheduling](notes/disk_scheduling.md)** - Disk I/O optimization and scheduling strategies
- **[File I/O and Filesystems](notes/file_io_and_filesystems.md)** - Descriptors, metadata, links, permissions, persistence
- **[Networking and Event Loops](notes/networking_and_event_loops.md)** - Framing, transports, readiness, bounded state
- **[Shells and Terminals](notes/shells_and_terminals.md)** - Pipelines, process groups, terminal control
- **[Service Lifecycle](notes/service_lifecycle.md)** - Readiness, logging, configuration reload, shutdown
- **[Security and Isolation](notes/security_and_isolation.md)** - Credentials, capabilities, seccomp, namespaces, cgroups
- **[Debugging and Performance](notes/debugging_and_performance.md)** - Tracing, debugging, sanitizer and profiling exercises
- **[Atomics and Memory Order](notes/atomics_and_memory_order.md)** - C11 atomicity and publication protocols
- **[Dynamic Loading and Mappings](notes/dynamic_loading_and_mappings.md)** - ABI and address ownership
- **[Asynchronous I/O](notes/asynchronous_io.md)** - io_uring completion model and buffer lifetime

These notes are perfect for reviewing concepts before tackling labs or preparing for exams.

## Getting Started

### Prerequisites

To work with the labs and examples in this repository, you'll need:

- **Operating System:** Linux (including WSL2). The process labs use anonymous shared mappings and process-shared POSIX semaphores; other Unix-like systems may need changes.
- **Compiler:** GCC (recommended) or Clang with C11 support
  ```sh
  # Install GCC on Ubuntu/Debian
  sudo apt update && sudo apt install build-essential

  # Verify installation
  gcc --version
  ```
- **POSIX Support:** POSIX threads library and semaphores (usually included in standard Linux distributions)
- **Tools:** Make and Python 3 for the complete build/regression workflow; command-line, editor, and Git basics
- **Optional tools:** `strace`, GDB, sanitizer runtimes, and `perf` for diagnostics; `pkg-config` and liburing development files for lab20. No optional library is required for `make all`.

### Quick Start

Here's how to get started with your first lab:

```sh
# Clone the repository (or your fork)
git clone https://github.com/djeada/Linux-System-Programming.git
cd Linux-System-Programming

# Navigate to lab1
cd labs/lab1

# Read the lab instructions
cat README.md

# Compile the program
gcc -std=c11 -Wall -Wextra -pedantic -o maze main.c -pthread

# Run the program with the provided maze
./maze maze.txt

# Try moving with W, A, S, D keys!
```

### General Build Pattern

Build and check the entire repository from its root:

```sh
make all
make comparisons
make test
make optional  # lab20, or an explicit missing-dependency skip
```

`make all` builds labs1–19 and the lab18 plugin. `make comparisons` builds the
lab3 fork/exec comparison and lab17 acquire/release publication exercise. The
lab3 default uses `posix_spawnp`; lab13 is the separate terminal job-control
shell. Keep `labs/common/` when copying a lab, since it contains shared helpers.
Headers are included by the programs, not compiled or run directly.

The tests build strict C11 binaries and check original and extended behaviors.
Networking tests bind temporary loopback ports/Unix paths; terminal tests use
a pseudo-terminal. A cgroup mutation test runs only with an explicitly delegated
`LAB_CGROUP_PARENT`. Namespace/optional io_uring restrictions report skip reasons.
The tests never install a service or change host kernel policy.

Most labs follow the same structure (see each lab README for exact commands):

```sh
cd labs/labX
cc -std=c11 -Wall -Wextra -pedantic -o labX main.c -pthread
./labX [args]
```

Lab18 also builds its shared plugin and links the host with `-ldl`; lab20 links
liburing. Use their Make targets or their dedicated README commands.

## Conventions

- **Language:** C11 with POSIX.1-2008 interfaces and documented Linux extensions; keep C examples in C.
- **Implementation path:** Prefer bounded work, explicit closure, checked startup, and owned worker lifetimes. Put API comparisons in a clearly labeled `comparisons/` directory.
- **Theory:** Label historical algorithms and kernel mechanisms separately from userspace implementation advice.
- **Compiler Flags:** Use `-Wall -Wextra -pedantic` for strict builds to catch potential issues.
- **Threading:** Labs involving threads require the `-pthread` flag during compilation.
- **Documentation:** Each lab includes a dedicated README with:
  - Learning objectives
  - Implementation constraints
  - API usage examples
  - Build and run instructions
- **Code Style:** Follow clean, readable code practices with meaningful variable names and comments where necessary.

## Troubleshooting

### Common Issues

**Compilation Errors:**
```sh
# If you get "undefined reference to pthread_create"
# Make sure to add -pthread flag
gcc -std=c11 -Wall -o program main.c -pthread

# If you get C11 feature errors
# Ensure your GCC version is 4.7 or newer
gcc --version
```

**Semaphore and Worker Issues:**

These labs use anonymous shared mappings, not persistent System V IPC objects.
They do not require global IPC cleanup. On an initialization or worker-startup
error, the reference implementation stops/reaps started processes or closes and
joins started threads before releasing shared state. `pthread_*` functions
return error numbers directly; their errors are printed with `strerror`.

For faster simulation runs, set `LAB_DELAY_MS` to a nonnegative millisecond
value. This changes simulated work only; elapsed-time deadlines remain real:

```sh
LAB_DELAY_MS=10 ./build/lab4 5 10
```

**Permission Denied:**
```sh
# If you can't execute the compiled program
chmod +x ./program_name
```

**Segmentation Faults:**
- Check array bounds and pointer arithmetic
- Ensure proper memory allocation/deallocation
- Use `gdb` for debugging: `gdb ./program_name`
- Compile with `-g` flag for debugging symbols

## Resources

### Books and References

- **[Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/)** - Comprehensive and freely available OS textbook
- **[Little OS Book](https://github.com/cstack/littleosbook)** - Practical guide to OS development
- **[OSDev Wiki](https://wiki.osdev.org/Expanded_Main_Page)** - Extensive OS development resources

### Online Courses

- **[Operating Systems Course Notes (UIC)](https://www.cs.uic.edu/~jbell/CourseNotes/OperatingSystems/)** - Detailed lecture notes covering OS fundamentals
- **[SOP2015 Operating Systems Course](https://mw.home.amu.edu.pl/zajecia/SOP2015/SOP.html)** - Comprehensive OS course materials

### Video Tutorials

- **[Linux Kernel Programming Playlist](https://www.youtube.com/watch?v=p-vqh0KBtHM&list=PLysdvSvCcUhbrU3HhGhfQVbhjnN9GXCq4)** - Deep dive into kernel programming
- **[Linux Programming Basics](https://www.youtube.com/watch?v=TB65-Kz8XvA&list=PLLqzlB47zvqiGOpRpwLQ3T13jHDOn7Mo-)** - Fundamental Linux programming concepts
- **[Comprehensive OS Playlist](https://youtube.com/playlist?list=PLsoEMNGAqtDOwDnR1W2AuTX79_iKPXRT7)** - Complete OS theory and practice
- **[Berkeley Linux Programming](https://youtube.com/playlist?list=PL6CdojO56mZ3SeRfpzMBMObSnTziA0gfE)** - University-level programming course

### Additional Tools

- **Manual Pages:** Use `man` command for detailed documentation (e.g., `man fork`, `man pthread_create`)
- **GDB Tutorial:** Learn debugging with [GDB Documentation](https://www.gnu.org/software/gdb/documentation/)
- **Valgrind:** Memory leak detection tool - [Valgrind Quick Start](https://valgrind.org/docs/manual/quick-start.html)

## Contributing

Contributions are welcome and appreciated! Here's how you can contribute:

### How to Contribute

1. **Fork the Repository** - Create your own fork of the project
2. **Create a Branch** - Make your changes in a new git branch
3. **Follow Conventions** - Maintain the existing structure and coding style:
   - Use C11 standard
   - Include clear comments and documentation
   - Follow the existing lab structure (README.md + main.c)
   - Ensure code compiles with `-Wall -Wextra -pedantic` without warnings
4. **Test Your Changes** - Run `make test` and compile with strict warnings; keep partial-startup and shutdown paths covered
5. **Submit a Pull Request** - Describe your changes and their purpose

### Contribution Ideas

- Add new labs covering different OS concepts
- Improve existing lab documentation
- Add more comprehensive notes on advanced topics
- Create additional quiz questions
- Fix bugs or improve code quality
- Add diagrams or visual aids to notes

### Guidelines

- Each new lab should include:
  - A detailed README with objectives, constraints, and instructions
  - A working reference implementation
  - Example input/output (if applicable)
  - Estimated difficulty level and completion time
- Keep the same structure for consistency
- Ensure all code is well-commented and follows best practices

## License

Licensed under the MIT License. See `LICENSE` for details.
