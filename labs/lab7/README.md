# Thread Pool and Work Queue

## Objective

Implement a fixed-size thread pool that processes tasks from a thread-safe work queue.

## Concepts Covered

- POSIX threads and synchronization
- Producer/consumer queues
- Condition variables and mutexes
- Safe parsing and task scheduling

## C APIs You Will Likely Use

- `pthread_create`, `pthread_join`
- `pthread_mutex_lock`, `pthread_mutex_unlock`, `pthread_mutex_destroy`
- `pthread_cond_wait`, `pthread_cond_signal`, `pthread_cond_broadcast`, `pthread_cond_destroy`
- `fopen`, `getline`, `printf`; validated exact comma-separated fields

## What You Must Implement

1. **Thread pool**
   - Create `N` worker threads (from the command line).
   - Each worker waits for tasks, processes them, and loops until shutdown.

2. **Work queue**
   - Store tasks in a fixed-capacity circular queue (32 tasks in the reference implementation).
   - Protect the queue with a mutex.
   - Use separate condition variables for available work and available capacity. Producers wait instead of allocating an unbounded backlog.

3. **Task input**
   - Read tasks from a file path provided on the command line.
   - Each line is a task, formatted as:
     ```
     TASK_ID,Description,Duration
     ```

4. **Shutdown**
   - Close the queue after input ends. Wake blocked producers and consumers; drain accepted tasks before workers exit.
   - Join all threads and clean up resources.

**Example Input** (`tasks.txt`)

```
1,Process user data,2
2,Render image,3
3,Send email,1
```

## Constraints and Edge Cases

- Skip malformed lines rather than crashing.
- Trim whitespace around fields if present.
- Duration must be a non-negative integer. Require exactly three unquoted fields and a nonempty description shorter than 256 bytes; reject extra or overlong fields.
- Track successfully created workers; on partial startup failure, close the queue and join only those workers. Pthread errors come from the returned error number.
- Avoid race conditions and spurious wakeups (use `while` with `pthread_cond_wait`).

## Deliverables

- `main.c`
- `README.md`
- `tasks.txt`

## Build and Run

```sh
cc -std=c11 -Wall -Wextra -pedantic -o thread_pool main.c -pthread
./thread_pool <num_workers> tasks.txt
```

Example:

```sh
./thread_pool 4 tasks.txt
```

## Implementation approach

The reference implementation follows the [C11/POSIX pattern guide](../../notes/modern_c_patterns.md).
Keep `labs/common/` with this lab. From the repository root, `make all` builds
all labs and `make test` checks behavior and startup/shutdown failures.

`LAB_DELAY_MS` can shorten simulated work delays for experiments; it does not change deadline clocks.
