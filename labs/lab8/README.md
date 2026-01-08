**Restaurant Order Processing Simulation**

**Objective**  
Build a concurrent restaurant simulation with **customers** (producers), **chefs** (consumers), and a **reporter** using threads, mutexes, and condition variables.

**Concepts Covered**

- Multi-threaded producer/consumer patterns
- Thread-safe queues with condition variables
- Coordinated shutdown
- Timed waits for periodic reporting

**C APIs You Will Likely Use**

- `pthread_create`, `pthread_join`
- `pthread_mutex_lock`, `pthread_mutex_unlock`, `pthread_mutex_destroy`
- `pthread_cond_wait`, `pthread_cond_signal`, `pthread_cond_broadcast`, `pthread_cond_timedwait`, `pthread_cond_destroy`
- `clock_gettime`, `time`, `sleep`, `usleep`
- `fopen`, `fgets`, `strtok_r`, `printf`

**What You Must Implement**

1. **Order queue**  
   - Shared, thread-safe queue of orders.  
   - Protected by a mutex and condition variable.

2. **Customer threads (producers)**  
   - Pull orders from an input file.  
   - Enqueue orders into the shared queue.  
   - Stop after the simulation duration or when orders run out.

3. **Chef threads (consumers)**  
   - Dequeue orders and process them (sleep for prep time).  
   - Update a shared counter of completed orders.

4. **Reporter thread**  
   - Periodically prints: waiting orders + total processed.
   - Exits when all work is done.

5. **Shutdown**  
   - After the simulation duration, customers stop producing.  
   - Chefs finish remaining orders and exit cleanly.  
   - All threads join before program exit.

**Input File Format**

Each line in the input file:
```
ORDER_ID,TableNumber,OrderDescription,PreparationTime
```
Example:
```
101,5,Pizza Margherita,3
102,2,Spaghetti Carbonara,2
103,8,Caesar Salad,1
```

**Constraints and Edge Cases**

- Skip malformed lines and continue processing.
- Trim whitespace around fields if present.
- Ensure chefs stop when producers are done and the queue is empty.
- Reporter should not block order processing.

**Deliverables**

- `main.c`
- `README.md`
- `orders.txt`

**Build and Run**

```sh
cc -std=c11 -Wall -Wextra -pedantic -o restaurant main.c -pthread
./restaurant <num_customers> <num_chefs> <input_file> <simulation_duration>
```

Example:

```sh
./restaurant 4 3 orders.txt 30
```
