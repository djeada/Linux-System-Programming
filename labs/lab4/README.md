**Sleeping Barber Simulation**

**Objective**  
Implement the classic Sleeping Barber synchronization problem using **processes**, **shared memory**, and **POSIX semaphores**.

**What You Must Implement**

1. **Shared state in shared memory**  
   Store at least:
   - number of waiting customers
   - number of waiting chairs
   - semaphores/mutexes needed for synchronization

2. **Barber process**  
   - Sleeps when there are no customers.
   - Wakes when a customer arrives.
   - Serves one customer at a time (simulate with `sleep`).
   - Exits cleanly after all customers are handled.

3. **Customer processes**  
   - Each customer checks for an available chair.
   - If full, the customer leaves immediately.
   - If a chair is available, the customer waits and is eventually served.

4. **Synchronization rules**  
   - Protect shared counters with a mutex.
   - Use semaphores to signal customer arrival and barber readiness.

5. **Command-line arguments**  
   - `argv[1]`: number of waiting chairs
   - `argv[2]`: number of customers to create

**Hints**

- Use `mmap()` with `MAP_SHARED | MAP_ANONYMOUS` for shared memory.
- Initialize semaphores with `pshared = 1` so processes share them.
- Avoid busy-wait loops; semaphores should block when no work is available.
- You will likely need at least:
  - a mutex for shared counters
  - a semaphore for available customers
  - a semaphore for the barber being ready

**Build and Run**

```sh
cc -std=c11 -Wall -Wextra -pedantic -o sleeping_barber main.c -pthread
./sleeping_barber <number_of_chairs> <number_of_customers>
```

Example:

```sh
./sleeping_barber 5 10
```
