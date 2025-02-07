# Scheduling Algorithms

Scheduling algorithms determine the order in which processes are executed by the CPU. Each algorithm has its own method of prioritizing tasks and optimizing various performance metrics such as waiting time, turnaround time, and overall system throughput. Here’s an overview of some common scheduling algorithms:

---

## 1. First Come First Serve (FCFS)

- **Description:**  
  The simplest scheduling algorithm where processes are scheduled in the order of their arrival in the ready queue.
- **Characteristics:**
  - **Non-preemptive:** Once a process starts executing, it runs to completion.
  - **Simple to implement.**
  - **Drawback:** Can result in long waiting times for processes that arrive later (convoy effect).

---

## 2. Shortest Job First (SJF)

- **Description:**  
  Processes with the shortest CPU burst time are scheduled first.
- **Characteristics:**
  - **Non-preemptive:** Once a process is selected, it runs until completion.
  - **Advantage:** Minimizes average waiting time if burst times are known.
  - **Drawback:** Can lead to starvation for longer processes if shorter processes keep arriving.

---

## 3. Shortest Remaining Time First (SRTF)

- **Description:**  
  The preemptive version of SJF. Processes are scheduled based on the shortest remaining CPU burst time.
- **Characteristics:**
  - **Preemptive:** A running process can be interrupted if a new process arrives with a shorter remaining time.
  - **Advantage:** More responsive than SJF in dynamic environments.
  - **Drawback:** Increased overhead due to frequent context switching.

---

## 4. Round Robin (RR) Scheduling

- **Description:**  
  Each process is assigned a fixed time slice (quantum) in a cyclic order.
- **Characteristics:**
  - **Preemptive:** Processes are preempted when their time quantum expires.
  - **Fairness:** Provides a balanced approach for all processes.
  - **Usage:** Ideal for time-sharing systems.
  - **Note:** The performance is highly dependent on the chosen time quantum.

---

## 5. Priority-Based Scheduling (Non-Preemptive)

- **Description:**  
  Processes are scheduled based on their priority; the process with the highest priority is selected first. If two processes have the same priority, they are scheduled according to their arrival time.
- **Characteristics:**
  - **Non-preemptive:** Processes run to completion once selected.
  - **Drawback:** Can lead to starvation for lower-priority processes if higher-priority processes continuously arrive.
  - **Alternative:** A preemptive version exists where processes can be interrupted if a higher priority process becomes available.

---

## 6. Highest Response Ratio Next (HRRN)

- **Description:**  
  Processes are scheduled based on the highest response ratio, balancing waiting time and burst time to reduce starvation.
- **Formula:**  
  ```
  Response Ratio = (Waiting Time + Burst Time) / Burst Time
  ```
- **Characteristics:**
  - **Advantage:** Reduces the risk of starvation while considering both burst time and waiting time.
  - **Usage:** Provides a more balanced approach in systems with diverse process workloads.

---

## 7. Multilevel Queue Scheduling (MLQ)

- **Description:**  
  Processes are divided into multiple queues based on specific criteria such as priority or process type.
- **Characteristics:**
  - **Fixed Assignment:** Processes are permanently assigned to a queue based on their attributes.
  - **Hierarchical:** Higher priority queues are always scheduled before lower priority queues.
  - **Drawback:** Lack of flexibility; processes cannot move between queues.

---

## 8. Multilevel Feedback Queue Scheduling (MLFQ)

- **Description:**  
  An advanced version of MLQ that allows processes to move between queues based on their behavior and CPU burst characteristics.
- **Characteristics:**
  - **Dynamic Adjustment:** Processes that consume too much CPU time are moved to a lower-priority queue.
  - **Advantage:** Improves fairness and responsiveness by dynamically adapting to process behavior.
  - **Usage:** Common in modern operating systems to balance system responsiveness and throughput.
