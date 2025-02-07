**parking garage**

Below is a complete implementation for the following task. In this task you will simulate a parking garage that can accommodate a limited number of vehicles at a time—but only vehicles of the same type may park together. Each vehicle is represented by a separate process. The shared memory region maintains the current state of the garage (including the number of parked vehicles, total parking spots, and the type of vehicle currently occupying the garage), and a semaphore is used to protect access to this shared state. Additionally, the program reads vehicle data (vehicle type and model name) from a file (named `vehicles.txt`); make sure the path to that file is specified correctly in your source code.

- **Objective:**  
  Learn how to use shared memory and semaphores to synchronize multiple processes. In this simulation:
  - The parking garage has a fixed number of parking spots (provided as the first command-line argument).
  - A list of vehicles (with type and model name) is read from a file named `vehicles.txt`.
  - Each vehicle is represented by a process that attempts to park.
  - **Rules for Parking:**
    - If the garage is empty or currently occupied by vehicles of the same type, a vehicle will park if there is an available spot.
    - If the garage is occupied by vehicles of a different type, the vehicle must wait until the garage is completely empty.
  - Once parked, a vehicle will simulate being parked for a short random duration before leaving, after which the shared state is updated.

- **Input File:**  
  The file `vehicles.txt` should contain one vehicle per line in the following format:
  ```
  C Toyota
  T Freightliner
  C Honda
  T Volvo
  ```
  Here, the first letter indicates the type of the vehicle (for example, 'C' for car and 'T' for truck), and the following string is the model name. The number of lines read should match (or exceed) the total number of vehicles specified as the second command-line argument.

- **Compilation and Execution Instructions:**
  - **Compilation:**  
    Use the standard GCC command (link with `-pthread` if needed):
    ```bash
    gcc parking_garage.c -o parking_garage -pthread
    ```
  - **Execution:**  
    Run the executable by supplying two arguments:
    ```bash
    ./parking_garage <number_of_spots> <number_of_vehicles>
    ```
    For example:
    ```bash
    ./parking_garage 5 10
    ```

cess communication—all within a creative and practical simulation scenario. Happy coding!
