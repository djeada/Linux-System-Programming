# Linux System Programming

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language: C](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))

A comprehensive, hands-on repository for learning Linux system programming through practical labs, detailed notes, and quizzes. Each lab focuses on fundamental OS concepts (processes, threads, synchronization, IPC) with clear instructions and reference implementations in modern C.

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

This repository is designed for students and developers who want to gain practical experience with Linux system programming. It covers essential topics including process management, inter-process communication, threading, synchronization mechanisms, and more. Each lab is self-contained with detailed instructions, and the notes section provides theoretical background to complement the hands-on exercises.

## Repository Layout

- `labs/` - programming assignments with `main.c` and lab-specific `README.md`
- `notes/` - course notes and reference materials
- `quizzes/` - quick checks and practice questions
- `LICENSE` - MIT license

## Labs Overview

Each lab includes a dedicated README with objectives, constraints, and build/run instructions.

| Lab | Topic | Core Concepts | Difficulty | Est. Time |
| --- | --- | --- | --- | --- |
| [lab1](labs/lab1) | Maze Navigator | signals, timeouts, input handling | Beginner | 2-3 hours |
| [lab2](labs/lab2) | Forked File Reader | `fork`, independent file I/O | Beginner | 1-2 hours |
| [lab3](labs/lab3) | Mini Shell | parsing, `fork`, `execvp`, `waitpid` | Intermediate | 3-4 hours |
| [lab4](labs/lab4) | Sleeping Barber | shared memory, semaphores, IPC | Advanced | 4-6 hours |
| [lab5](labs/lab5) | Text Pipeline | multi-process pipeline, shared buffers | Advanced | 4-5 hours |
| [lab6](labs/lab6) | Parking Garage | synchronization policy, fairness | Advanced | 4-6 hours |
| [lab7](labs/lab7) | Thread Pool | worker threads, work queues | Intermediate | 3-4 hours |
| [lab8](labs/lab8) | Restaurant Simulation | producers/consumers, reporting | Advanced | 4-5 hours |

## Notes

The `notes/` directory contains comprehensive course materials covering theoretical concepts that complement the hands-on labs:

- **[Operating Systems](notes/operating_systems.md)** - Overview of OS types, responsibilities, and fundamental concepts
- **[Processes and Threads](notes/processes_and_threads.md)** - Process lifecycle, scheduling, and thread management
- **[Memory Management](notes/memory_management.md)** - Virtual memory, paging, segmentation, and memory allocation strategies
- **[Synchronization Tools](notes/synchronization_tools.md)** - Mutexes, semaphores, and other synchronization primitives
- **[Critical Section Problem](notes/critical_section_problem.md)** - Race conditions, mutual exclusion, and solutions
- **[Scheduling Algorithms](notes/scheduling_algorithms.md)** - CPU scheduling policies and algorithms
- **[Disk Scheduling](notes/disk_scheduling.md)** - Disk I/O optimization and scheduling strategies

These notes are perfect for reviewing concepts before tackling labs or preparing for exams.

## Getting Started

### Prerequisites

To work with the labs and examples in this repository, you'll need:

- **Operating System:** Linux or Unix-like environment (Ubuntu, Debian, macOS, WSL on Windows)
- **Compiler:** GCC (recommended) or Clang with C11 support
  ```sh
  # Install GCC on Ubuntu/Debian
  sudo apt update && sudo apt install build-essential
  
  # Verify installation
  gcc --version
  ```
- **POSIX Support:** POSIX threads library and semaphores (usually included in standard Linux distributions)
- **Tools:** Basic familiarity with command line, text editor, and Git

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
gcc -std=c11 -Wall -Wextra -pedantic -o maze main.c

# Run the program with the provided maze
./maze maze.txt

# Try moving with W, A, S, D keys!
```

### General Build Pattern

Most labs follow the same structure (see each lab README for exact commands):

```sh
cd labs/labX
cc -std=c11 -Wall -Wextra -pedantic -o labX main.c -pthread
./labX [args]
```

## Conventions

- **Language:** All code is written in C11 standard (no C++).
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

**Semaphore Issues:**
```sh
# If semaphore operations fail, you may need to increase system limits
# Check current limits
ipcs -l

# Remove stale semaphores (if needed)
ipcrm -a
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
4. **Test Your Changes** - Verify that your code works correctly
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
