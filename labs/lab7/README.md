**Thread Pool and Work Queue**

**Objective**  
Implement a fixed-size thread pool that processes tasks from a thread-safe work queue.

**Concepts Covered**

- POSIX threads and synchronization
- Producer/consumer queues
- Condition variables and mutexes
- Safe parsing and task scheduling

**C APIs You Will Likely Use**

- `pthread_create`, `pthread_join`
- `pthread_mutex_lock`, `pthread_mutex_unlock`, `pthread_mutex_destroy`
- `pthread_cond_wait`, `pthread_cond_signal`, `pthread_cond_broadcast`, `pthread_cond_destroy`
- `fopen`, `fgets`, `strtok_r`, `printf`

**What You Must Implement**

1. **Thread pool**  
   - Create `N` worker threads (from the command line).  
   - Each worker waits for tasks, processes them, and loops until shutdown.

2. **Work queue**  
   - Store tasks in a shared queue (linked list or circular buffer).  
   - Protect the queue with a mutex.  
   - Use a condition variable to wake workers when tasks arrive.

3. **Task input**  
   - Read tasks from a file path provided on the command line.  
   - Each line is a task, formatted as:  
     ```
     TASK_ID,Description,Duration
     ```

4. **Shutdown**  
   - After all tasks are enqueued, signal workers to exit once the queue is empty.  
   - Join all threads and clean up resources.

**Example Input** (`tasks.txt`)

```
1,Process user data,2
2,Render image,3
3,Send email,1
```

**Constraints and Edge Cases**

- Skip malformed lines rather than crashing.
- Trim whitespace around fields if present.
- Duration must be a non-negative integer.
- Ensure workers exit when the queue is empty and shutdown is signaled.
- Avoid race conditions and spurious wakeups (use `while` with `pthread_cond_wait`).

**Deliverables**

- `main.c`
- `README.md`
- `tasks.txt`

**Build and Run**

```sh
cc -std=c11 -Wall -Wextra -pedantic -o thread_pool main.c -pthread
./thread_pool <num_workers> tasks.txt
```

Example:

```sh
./thread_pool 4 tasks.txt
```
