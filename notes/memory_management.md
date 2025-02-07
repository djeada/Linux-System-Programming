
# Memory Management

Memory management techniques enable the efficient sharing and allocation of memory among multiple processes. This section covers various methods for managing memory, including partitioning, paging, segmentation, and page replacement strategies.

## 1. Memory Sharing Techniques

### Overlays
- **Concept:**  
  Only the instructions and data needed at a given time are loaded into memory.
- **Usage:**  
  Used in environments with limited memory, ensuring that only a portion of a large program is resident in memory at any one time.

### Swapping
- **Concept:**  
  In multiprogramming systems, processes that have used their allotted time slice are temporarily swapped out from main memory.
- **Usage:**  
  Helps in managing multiple processes by moving inactive processes to secondary storage and bringing them back when needed.

---

## 2. Memory Allocation Schemes

Memory can be allocated to processes using various partitioning schemes:

### a. Single Partition Allocation Scheme
- **Description:**  
  Memory is divided into two distinct parts:
  - One part reserved for the operating system.
  - The other part allocated to user processes.

### b. Multiple Partition Allocation Schemes
Memory is divided into partitions that can be either fixed or variable in size.

#### 1. Fixed Partitioning
- **Description:**  
  Memory is divided into fixed-size partitions.
- **Characteristics:**  
  - Simplicity in design.
  - Can lead to internal fragmentation if a process does not fully utilize its partition.

#### 2. Variable Partitioning
- **Description:**  
  Memory is divided into partitions of variable sizes based on the size of the process.
- **Allocation Strategies:**

- **First Fit:**  
  The arriving process is allocated to the first available memory hole in which it fits completely.
  
- **Best Fit:**  
  The process is allocated to the smallest hole that is large enough, minimizing wasted space.  
  *Note:* Best Fit does not always yield the most efficient memory utilization in practice.
  
- **Worst Fit:**  
  The process is allocated to the largest available hole, leaving the maximum leftover space.

> **Note:**  
> Both fixed and variable partitioning require that each process be allocated a contiguous block of memory. This can lead to external fragmentation, which is why more advanced techniques like paging are often used.

---

## 3. Advanced Memory Management Techniques

### Paging
- **Concept:**  
  The physical memory is divided into fixed-size blocks called *frames*, and the logical memory is divided into fixed-size blocks called *pages*.  
- **Key Point:**  
  The size of a physical frame is equal to the size of a logical page, allowing non-contiguous memory allocation and reducing external fragmentation.

### Segmentation
- **Concept:**  
  Provides a logical view of memory where the address space is divided into segments (e.g., code, data, stack).  
- **Implementation:**  
  Segmentation can be used alone or in combination with paging to improve memory utilization and provide better support for varying process sizes.
  
---

## 4. Page Faults

- **Definition:**  
  A **page fault** is an interrupt triggered when a running program accesses a memory page that is mapped in the virtual address space but not currently loaded into physical memory.
- **Handling:**  
  When a page fault occurs, the operating system must locate the required page (possibly from secondary storage), load it into a free frame, and update the page table before resuming the program.

---

## 5. Page Replacement Algorithms

When physical memory is full, the operating system must decide which page to remove to make room for a new page. The following are common page replacement algorithms:

### 5.1 First In First Out (FIFO)
- **Mechanism:**  
  - Pages are maintained in a queue in the order they were loaded into memory.
  - The oldest page (at the front of the queue) is selected for replacement when a new page is needed.
- **Example:**  
  Consider the page reference string `1, 3, 0, 3, 5, 6` with 3 page slots:
  - **Step 1:** Pages `1`, `3`, and `0` are loaded into the empty slots → **3 page faults**.
  - **Step 2:** The next `3` is already in memory → **0 page faults**.
  - **Step 3:** When page `5` arrives, it replaces the oldest page (`1`) → **1 page fault**.
  - **Step 4:** When page `6` arrives, it replaces the next oldest page (`3`) → **1 page fault**.
- **Belady’s Anomaly:**  
  FIFO can exhibit Belady’s anomaly, where increasing the number of page frames results in more page faults.  
  *Example:* With a specific reference string, using 3 frames might result in 9 faults, but increasing to 4 frames could result in 10 faults.

---

### 5.2 Optimal Page Replacement
- **Mechanism:**  
  - Replaces the page that will not be used for the longest period of time in the future.
- **Characteristics:**  
  - This algorithm is theoretical because it requires future knowledge of the page reference string.
  - It is used as a benchmark to evaluate other page replacement algorithms.
- **Example:**  
  Consider the page reference string `7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2` with 4 page slots:
  - **Step 1:** Pages `7`, `0`, `1`, `2` are loaded → **4 page faults**.
  - **Step 2:** The next page `0` is already in memory → **0 page fault**.
  - **Step 3:** Page `3` replaces `7` (as `7` is not needed for the longest time in the future) → **1 page fault**.
  - **Step 4:** Continue this process by replacing the page that is not needed for the longest future period.

---

### 5.3 Least Recently Used (LRU)
- **Mechanism:**  
  - Replaces the page that has not been used for the longest period of time.
- **Characteristics:**  
  - Assumes that pages used recently will likely be used again soon.
- **Example:**  
  For the page reference string `7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2` with 4 page slots:
  - **Step 1:** Load `7`, `0`, `1`, `2` → **4 page faults**.
  - **Step 2:** When page `3` arrives, it replaces page `7` (the least recently used) → **1 page fault**.
  - **Step 3:** Continue this process by always replacing the page that has not been used for the longest time.

