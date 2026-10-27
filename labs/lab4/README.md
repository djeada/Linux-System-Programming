# Sleeping Barber Simulation

## Objective

Implement the classic Sleeping Barber synchronization problem using **processes**, **shared memory**, and **POSIX semaphores**.

## What You Must Implement

1. **Shared state in shared memory**
   Store at least:
   - number of waiting customers
   - number of waiting chairs
   - FIFO waiting-customer links and a completion semaphore per customer
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
   - Use a semaphore for customer arrivals and a separate completion semaphore for each customer. A customer cannot pay until its own haircut finishes.
   - Account for all arrivals so the barber exits after accepted work drains, including zero-chair and zero-customer cases.

5. **Command-line arguments**
   - `argv[1]`: number of waiting chairs
   - `argv[2]`: number of customers to create

## Hints

- Use `mmap()` with `MAP_SHARED | MAP_ANONYMOUS` for shared memory.
- Initialize semaphores with `pshared = 1` so processes share them.
- Avoid busy-wait loops; semaphores should block when no work is available.
- Use a mutex for the FIFO/counters, an arrival semaphore, and per-customer completion gates.
- Check each initialization and fork. Roll back partial semaphore setup and terminate/reap started peers if a worker fails.

## Build and Run

```sh
cc -std=c11 -Wall -Wextra -pedantic -o sleeping_barber main.c -pthread
./sleeping_barber <number_of_chairs> <number_of_customers>
```

Example:

```sh
./sleeping_barber 5 10
```

## Implementation approach

The reference implementation follows the [C11/POSIX pattern guide](../../notes/modern_c_patterns.md).
Keep `labs/common/` with this lab. From the repository root, `make all` builds
all labs and `make test` checks behavior and startup/shutdown failures.

`LAB_DELAY_MS` can shorten simulated work delays for experiments; it does not change deadline clocks.
