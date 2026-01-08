**Parking Garage Simulation**

**Objective**  
Simulate a parking garage with **processes** and **shared memory**, enforcing a rule that only **one vehicle type** may park at a time.

**Concepts Covered**

- Process synchronization
- Shared memory (`mmap`) across processes
- POSIX semaphores and mutual exclusion
- Fair access policies

**C APIs You Will Likely Use**

- `fork`, `waitpid`, `getpid`
- `mmap`, `munmap`
- `sem_init`, `sem_wait`, `sem_post`, `sem_destroy`
- `fopen`, `fgets`, `sscanf`, `printf`

**What You Must Implement**

1. **Shared state**  
   Use shared memory to store:
   - current vehicle type inside the garage
   - number of vehicles parked
   - number of waiting vehicles (per type)
   - total spots
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

**Constraints and Edge Cases**

- Reject invalid arguments (non-positive or malformed numbers).
- Handle cases where the input file has fewer valid vehicles than requested.
- Avoid busy-waiting; blocked vehicles should wait on semaphores.

**Deliverables**

- `main.c`
- `README.md`
- `vehicles.txt`

**Build and Run**

```sh
cc -std=c11 -Wall -Wextra -pedantic -o parking_garage main.c -pthread
./parking_garage <number_of_spots> <number_of_vehicles> [vehicles_file]
```

Example:

```sh
./parking_garage 5 10 vehicles.txt
```
