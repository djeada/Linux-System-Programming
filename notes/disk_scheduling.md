# Disk Scheduling

Disk scheduling is the process by which an operating system orders disk I/O requests to optimize performance. Since disk access involves mechanical movement (such as moving the read/write head), minimizing delays through efficient scheduling is crucial.

---

## Disk Access Parameters

1. **Seek Time:**  
   The time taken to move the disk’s read/write head to the track where the desired data is located.

2. **Rotational Latency:**  
   The time it takes for the desired sector of the disk to rotate into position under the read/write head.

3. **Transfer Time:**  
   The time required to transfer the data once the head is in position. This depends on the disk’s rotational speed and the size of the data.

4. **Disk Access Time:**  
   The total time required to access data, calculated as:  
   ```
   Disk Access Time = Seek Time + Rotational Latency + Transfer Time
   ```

5. **Disk Response Time:**  
   The average time a disk request waits before being serviced. It is computed as the average of the waiting times for all requests in the disk queue.

---

## Disk Scheduling Algorithms

Different algorithms optimize disk scheduling in various ways:

### 1. First-Come, First-Served (FCFS)

- **Description:**  
  Requests are processed in the exact order in which they arrive in the disk queue.

- **Advantages:**  
  - Simple to implement.  
  - Fair in terms of arrival order.

- **Drawbacks:**  
  - May result in long wait times if requests are spread across distant areas of the disk.

---

### 2. Shortest Seek Time First (SSTF)

- **Description:**  
  The scheduler selects the request that requires the least movement (i.e., the shortest seek time) from the current head position.

- **Advantages:**  
  - Reduces overall seek time.
  
- **Drawbacks:**  
  - Can lead to starvation for requests that are far from the current head position.

---

### 3. SCAN (Elevator Algorithm)

- **Description:**  
  The disk arm moves in one direction, servicing all requests along its path. Upon reaching the last request in that direction, it reverses and services requests on the return path.

- **Advantages:**  
  - Provides a more uniform wait time compared to FCFS.
  
- **Drawbacks:**  
  - Requests at the extreme ends of the disk may experience longer wait times.

---

### 4. C-SCAN (Circular SCAN)

- **Description:**  
  Similar to SCAN, but the disk arm only services requests in one direction. When it reaches the end of the disk, it quickly returns to the beginning without servicing requests during the return, and then resumes servicing in the same direction.

- **Advantages:**  
  - Ensures a more uniform wait time by preventing the head from biasing toward one end of the disk.
  
- **Drawbacks:**  
  - The return movement is unserviced, which can be considered wasted time.

---

### 5. LOOK

- **Description:**  
  A variant of SCAN. Instead of going to the physical end of the disk, the arm only travels as far as the last request in its current direction before reversing.

- **Advantages:**  
  - Reduces unnecessary movement, potentially lowering wait times.
  
- **Drawbacks:**  
  - May slightly vary in performance depending on the distribution of requests.

---

### 6. C-LOOK (Circular LOOK)

- **Description:**  
  A variant of C-SCAN. The disk arm goes only as far as the last request in one direction and then immediately jumps back to the first request on the other end, instead of traversing to the very end of the disk.

- **Advantages:**  
  - Further minimizes unnecessary traversal.
  - Provides a uniform wait time by ensuring that requests are handled in a circular fashion.
  
- **Drawbacks:**  
  - Slightly more complex to implement compared to basic SCAN.

