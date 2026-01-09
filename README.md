# Linux System Programming

A hands-on repository of Linux system programming labs, notes, and quizzes. Each lab focuses on a core OS concept (processes, threads, synchronization, IPC) with clear student-facing instructions and reference implementations in modern C.

![linux_system_programming](https://github.com/user-attachments/assets/f3e18e63-7e2c-4705-b400-37fa533f6d35)

## Repository Layout

- `labs/` - programming assignments with `main.c` and lab-specific `README.md`
- `notes/` - course notes and reference materials
- `quizzes/` - quick checks and practice questions
- `LICENSE` - MIT license

## Labs Overview

| Lab | Topic | Core Concepts |
| --- | --- | --- |
| lab1 | Maze Navigator | signals, timeouts, input handling |
| lab2 | Forked File Reader | `fork`, independent file I/O |
| lab3 | Mini Shell | parsing, `fork`, `execvp`, `waitpid` |
| lab4 | Sleeping Barber | shared memory, semaphores, IPC |
| lab5 | Text Pipeline | multi-process pipeline, shared buffers |
| lab6 | Parking Garage | synchronization policy, fairness |
| lab7 | Thread Pool | worker threads, work queues |
| lab8 | Restaurant Simulation | producers/consumers, reporting |

## Getting Started

**Prerequisites**

- Linux or Unix-like environment
- C compiler (`gcc`/`clang`)
- POSIX threads and semaphores support

**General Build Pattern**

Most labs follow the same structure (see each lab README for exact commands):

```sh
cd labs/labX
cc -std=c11 -Wall -Wextra -pedantic -o labX main.c -pthread
./labX [args]
```

## Conventions

- Code is written in C11 (no C++).
- Use `-Wall -Wextra -pedantic` for strict builds.
- Threaded labs require `-pthread`.
- Each lab README includes constraints, APIs, and example usage.

## Resources

**Books and References**

- [Operating Systems: Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/)
- [Little OS Book](https://github.com/cstack/littleosbook)
- [OSDev Wiki](https://wiki.osdev.org/Expanded_Main_Page)

**Courses and Notes**

- [Operating Systems Course Notes (UIC)](https://www.cs.uic.edu/~jbell/CourseNotes/OperatingSystems/)
- [SOP2015 Operating Systems Course](https://mw.home.amu.edu.pl/zajecia/SOP2015/SOP.html)

**Videos and Playlists**

- [Linux Kernel Programming Playlist](https://www.youtube.com/watch?v=p-vqh0KBtHM&list=PLysdvSvCcUhbrU3HhGhfQVbhjnN9GXCq4)
- [Additional Linux Programming Videos](https://www.youtube.com/watch?v=TB65-Kz8XvA&list=PLLqzlB47zvqiGOpRpwLQ3T13jHDOn7Mo-)
- [Comprehensive OS Playlist](https://youtube.com/playlist?list=PLsoEMNGAqtDOwDnR1W2AuTX79_iKPXRT7)
- [Berkeley Linux Programming Playlist](https://youtube.com/playlist?list=PL6CdojO56mZ3SeRfpzMBMObSnTziA0gfE)

## Contributing

Contributions are welcome. If you add new labs or improve existing ones, keep the same structure and include a concise lab README with objectives, constraints, and build/run steps.

## License

Licensed under the MIT License. See `LICENSE` for details.
