# Restaurant Order Processing Simulation

## Objective

Build a concurrent restaurant simulation with **customers** (producers), **chefs** (consumers), and a **reporter** using threads, mutexes, and condition variables.

## Concepts Covered

- Multi-threaded producer/consumer patterns
- Thread-safe queues with condition variables
- Coordinated shutdown
- Timed waits for periodic reporting

## C APIs You Will Likely Use

- `pthread_create`, `pthread_join`
- `pthread_mutex_lock`, `pthread_mutex_unlock`, `pthread_mutex_destroy`
- `pthread_cond_wait`, `pthread_cond_signal`, `pthread_cond_broadcast`, `pthread_cond_timedwait`, `pthread_cond_destroy`
- `clock_gettime(CLOCK_MONOTONIC)`, `nanosleep`
- `fopen`, `getline`, validated exact comma-separated fields, `printf`

## What You Must Implement

1. **Order queue**
   - Shared, thread-safe ring queue of 32 orders; a full queue blocks producers.
   - Protected by a mutex and condition variable.

2. **Customer threads (producers)**
   - Pull orders from an input file.
   - Enqueue orders into the shared queue.
   - Stream records from the input under a source mutex; do not preload the file or allocate per-order queue nodes.
   - Stop after the monotonic deadline or when orders run out, including when a producer is waiting for queue capacity.

3. **Chef threads (consumers)**
   - Dequeue orders and process them (sleep for prep time).
   - Update a shared counter of completed orders.

4. **Reporter thread**
   - Periodically prints: waiting orders + total processed.
   - Uses a monotonic timed condition wait and emits its final report only after all chefs have joined. An empty queue can still have orders in progress.

5. **Shutdown**
   - After the simulation duration, customers stop producing.
   - Chefs finish remaining orders and exit cleanly.
   - Start chefs before customers. If startup partially fails, close the queue before joining any producer.
   - All threads join before program exit, then destroy queues and synchronization objects.

## Input File Format

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

## Constraints and Edge Cases

- Skip malformed lines and continue processing. Require exactly four unquoted, nonempty fields; reject extra fields and descriptions of 256 bytes or more.
- Trim whitespace around fields if present.
- Ensure chefs stop when producers are done and the queue is empty.
- Reporter should not block order processing.

## Deliverables

- `main.c`
- `README.md`
- `orders.txt`

## Build and Run

```sh
cc -std=c11 -Wall -Wextra -pedantic -o restaurant main.c -pthread
./restaurant <num_customers> <num_chefs> <input_file> <simulation_duration>
```

Example:

```sh
./restaurant 4 3 orders.txt 30
```

## Implementation approach

The reference implementation follows the [C11/POSIX pattern guide](../../notes/modern_c_patterns.md).
Keep `labs/common/` with this lab. From the repository root, `make all` builds
all labs and `make test` checks behavior and startup/shutdown failures.

`LAB_DELAY_MS` can shorten simulated work delays for experiments; it does not change deadline clocks.
