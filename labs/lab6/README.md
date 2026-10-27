# Parking Garage Simulation

## Objective

Simulate a parking garage with **processes** and **shared memory**, enforcing a rule that only **one vehicle type** may park at a time.

## Concepts Covered

- Process synchronization
- Shared memory (`mmap`) across processes
- POSIX semaphores and mutual exclusion
- Fair access policies

## C APIs You Will Likely Use

- `fork`, `waitpid`, `getpid`
- `mmap`, `munmap`
- `sem_init`, `sem_wait`, `sem_post`, `sem_destroy`
- `fopen`, `fgets`, `sscanf`, `printf`

## What You Must Implement

1. **Shared state**
   Use shared memory to store:
   - current vehicle type inside the garage
   - number of vehicles parked
   - number of waiting vehicles (per type)
   - total spots
   - FIFO waiters and a private admission gate for every vehicle
   - semaphores for synchronization

2. **Vehicle processes**
   Each vehicle is a process that:
   - arrives, attempts to park, and waits if necessary
   - parks for a random duration
   - exits and updates shared state

3. **Parking rules**
   - If the garage is empty, the arriving vehicle sets the type.
   - If the garage is occupied by the **same type**, vehicles may enter until spots are full.
   - If the garage is occupied by a **different type**, vehicles must wait until the garage is empty.

4. **Input file**
   Read vehicles from `vehicles.txt`. Each line:
   ```
   C Toyota
   T Freightliner
   ```
   `C` = car, `T` = truck, followed by a model name (one word).

5. **Command-line arguments**
   - `argv[1]`: number of parking spots
   - `argv[2]`: number of vehicles to simulate
   - `argv[3]` (optional): path to vehicles file (default: `vehicles.txt`)

## Constraints and Edge Cases

- Reject invalid arguments (non-positive or malformed numbers).
- Handle cases where the input file has fewer valid vehicles than requested.
- Avoid busy-waiting; each blocked vehicle waits on its own admission gate.
- Reserve each slot under the mutex **before** posting its gate, so a new arrival cannot claim the same capacity.
- Once the opposite type is waiting, stop extending the current batch; alternate type batches while both types wait.
- Check process startup and reap all participants before destroying their shared state.

## Deliverables

- `main.c`
- `README.md`
- `vehicles.txt`

## Build and Run

```sh
cc -std=c11 -Wall -Wextra -pedantic -o parking_garage main.c -pthread
./parking_garage <number_of_spots> <number_of_vehicles> [vehicles_file]
```

Example:

```sh
./parking_garage 5 10 vehicles.txt
```

## Implementation approach

The reference implementation follows the [C11/POSIX pattern guide](../../notes/modern_c_patterns.md).
Keep `labs/common/` with this lab. From the repository root, `make all` builds
all labs and `make test` checks behavior and startup/shutdown failures.

`LAB_DELAY_MS` can shorten simulated work delays for experiments; it does not change deadline clocks.
