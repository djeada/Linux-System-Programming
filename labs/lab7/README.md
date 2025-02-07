 **Task: Thread Pool and Work Queue Processing**

Design and implement a thread pool in C that accepts and processes tasks concurrently. You will:

- Create a fixed pool of worker threads.
- Implement a thread-safe work queue that holds tasks.
- Use mutexes and condition variables to synchronize task enqueuing and dequeuing.
- Dynamically add tasks to the work queue from an input file.
- Ensure that all tasks are processed before shutting down the thread pool gracefully.

### **Task Description**

1. **Thread Pool Setup:**
   - **Worker Threads:**  
     The program should create a fixed number of worker threads (specified as a command-line argument).  
     Each worker thread should run an infinite loop waiting for tasks to be available in the work queue.
   - **Work Queue:**  
     Implement the work queue using a data structure of your choice (for example, a linked list or an array-based circular buffer).  
     This queue holds “tasks” where each task is a structure containing at least a task ID and a brief description (or command string).  
     The main thread (producer) will enqueue tasks, and worker threads (consumers) will dequeue and execute tasks.

2. **Task Input and Enqueuing:**
   - The program should accept a second command-line argument specifying the path to an input file.
   - Each line in the input file represents a task (for example, a string message to print and a numeric value to simulate work duration).
   - The main thread reads each line, creates a task structure, and enqueues it into the work queue.
   - Use proper locking (via mutex) when modifying the queue, and signal waiting worker threads (via a condition variable) that a new task is available.

3. **Task Execution by Worker Threads:**
   - Worker threads should continuously wait for tasks.
   - When a task becomes available, a worker thread should:
     - Remove the task from the queue.
     - Print the task’s details along with the worker thread’s ID.
     - Simulate performing work by sleeping for a duration specified in the task (or a random duration if not specified).
   - After completing the task, the thread should check for more tasks.
   - Design the worker loop so that it can be terminated gracefully once all tasks have been processed.

4. **Shutdown and Cleanup:**
   - Once the main thread has read all tasks from the input file and enqueued them, it should signal the worker threads that no further tasks will arrive (e.g., by setting a “shutdown” flag in shared memory).
   - Worker threads should exit their loop and join back into the main thread.
   - Finally, clean up all allocated resources (e.g., free dynamic memory, destroy mutexes and condition variables).

5. **Synchronization:**
   - Use a mutex to protect shared data (the work queue and any global flags).
   - Use one or more condition variables to allow worker threads to wait for new tasks and to signal when the work queue is non-empty.
   - Ensure that the program is free of race conditions and deadlocks.

---

### **Compilation and Execution Instructions**

- **Compilation:**
  - Use the GNU Compiler Collection (gcc) with the appropriate flags (linking with `-pthread` is required).
  - Example compile command:
    ```bash
    gcc thread_pool.c -o thread_pool -pthread
    ```

- **Execution:**
  - The program should be executed with two command-line arguments:
    - The first argument specifies the number of worker threads.
    - The second argument specifies the path to an input file containing task definitions.
  - Example:
    ```bash
    ./thread_pool 5 tasks.txt
    ```
  - **Input File Format:**
    - Each line in the file might be formatted as:
      ```
      TASK_ID,Task description,Duration_in_seconds
      ```
    - For example:
      ```
      1,Process user data,2
      2,Render image,3
      3,Send email,1
      ```
    - You can choose to implement your own parsing logic based on your task structure.

---

### **Additional Considerations**

- **Dynamic Memory Management:**  
  Make sure to allocate and free any dynamic memory used by your work queue.
  
- **Error Handling:**  
  Handle error cases gracefully (e.g., file read errors, invalid input data, or thread creation failures).
  
- **Modular Design:**  
  Structure your program with clear, modular functions:
  - A function for enqueuing tasks.
  - A function for dequeuing tasks.
  - A worker function that the threads will execute.
  - A main function that sets up the thread pool, reads the input file, and manages shutdown.

- **Testing and Debugging:**  
  Test your implementation with various numbers of worker threads and tasks. Consider adding debug messages that print the current state of the work queue and the actions of each thread.

