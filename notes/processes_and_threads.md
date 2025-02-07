# Processes and Threads

## 1. Process

- **Definition:**  
  A **process** is a program in execution. It maintains its own execution context, which includes a **program counter (PC)** indicating the address of the next instruction to execute.

- **Process Control Block (PCB):**  
  Every process is represented by a PCB, a data structure that holds important information about the process, such as:
  - Process state
  - Register values
  - Memory pointers
  - I/O status information

---

## 2. Process Scheduling

Process scheduling is the method by which the operating system determines which process in the ready queue should be executed next. The following metrics are commonly used in process scheduling:

1. **Arrival Time:**  
   The time at which a process enters the ready queue.

2. **Completion Time:**  
   The time at which a process completes its execution.

3. **Burst Time:**  
   The amount of time required by a process for CPU execution.

4. **Turnaround Time:**  
   The total time taken from process arrival to its completion.  
   **Calculation:**  
   ```
   Turnaround Time = Completion Time - Arrival Time
   ```

5. **Waiting Time (WT):**  
   The time a process spends waiting in the ready queue, excluding the time during which it is executing.  
   **Calculation:**  
   ```
   Waiting Time = Turnaround Time - Burst Time
   ```

---

## 3. Threads

- **Definition:**  
  A **thread** is a lightweight unit of CPU utilization that exists within a process. Threads allow a process to perform multiple tasks concurrently.

- **Characteristics of Threads:**
  - **Individual Resources:**  
    Each thread has its own:
    - Program counter
    - Register set
    - Stack
  - **Shared Resources:**  
    Threads within the same process share the following:
    - Code section
    - Data section
    - Files
    - Signals

---

## 4. Creating Threads and Processes

- **Forking:**  
  A new thread or child process can be created using the `fork()` system call.  
  - **Note:**  
    If a process makes `n` calls to `fork()`, it can generate up to `2^n - 1` child processes.

---

## 5. Types of Threads

There are two primary types of threads:

1. **User Threads:**
   - Implemented at the user level (via user libraries).
   - Managed by the application rather than the kernel.

2. **Kernel Threads:**
   - Managed directly by the operating system.
   - The OS handles scheduling and execution, often resulting in better performance and resource management.

---

## 6. Multilevel Feedback Queue (MLFQ) Scheduling

- **Overview:**  
  MLFQ is a dynamic scheduling algorithm designed to optimize process scheduling based on CPU burst characteristics.

- **Key Features:**
  - **Queue Movement:**  
    Processes can move between different priority queues depending on their CPU usage.
  - **Priority Adjustment:**  
    If a process uses excessive CPU time, it is moved to a lower-priority queue, allowing shorter or less CPU-intensive processes to execute more promptly.

