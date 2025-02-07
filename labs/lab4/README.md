**Sleeping Barber Simulation**

### **Objective**

- **Learn Concurrency:**  
  Understand how to use shared memory, semaphores, and mutexes for synchronizing multiple processes.
  
- **Inter-Process Communication:**  
  Coordinate between a barber process and multiple customer processes in a concurrent environment.
  
- **Implement the Sleeping Barber Problem:**  
  Simulate a classic synchronization problem where:
  - The barber sleeps when no customers are present.
  - Each arriving customer checks for an available waiting chair.
  - If a chair is available, the customer sits and waits.
  - When the barber is free, a waiting customer is served.
  - After the haircut, the customer proceeds to pay (this can be simulated with a message).

### **Task Details**

1. **Shared Memory Setup:**  
   - Create a shared memory segment to hold a structure containing:
     - The number of waiting customers.
     - The total number of chairs (provided as a command-line argument).
     - Semaphores/mutexes for controlling access to shared variables.
     
2. **Semaphores and Mutexes:**  
   - Use a mutex to protect updates to shared variables.
   - Use semaphores to signal when:
     - A customer arrives.
     - The barber is ready.
     
3. **Processes:**
   - **Barber Process:**  
     - Runs in an infinite loop.
     - If there are no customers, the barber sleeps.
     - When a customer is available, the barber “cuts hair” (simulate with a sleep) and then signals that the haircut is complete.
   - **Customer Processes:**  
     - Each customer (process) first checks if there is a free waiting chair.
     - If no chair is available, the customer leaves.
     - If a chair is free, the customer sits, and if the barber is asleep, wakes him up.
     - After the haircut, the customer simulates payment (e.g., printing a message) and then exits.

4. **Program Flow:**  
   - The main process creates the shared memory and initializes semaphores.
   - It then spawns the barber process.
   - Next, it spawns a process for each customer (the total number of customers is specified via a command-line argument).
   - The main process waits for all customer processes to complete, then terminates the simulation.

5. **Command-Line Arguments:**  
   - **First argument:** Number of waiting chairs.
   - **Second argument:** Total number of customers to simulate.

6. **Compile and Execution Instructions:**  
   - **Compilation:**  
     Compile with the standard GCC command. (You may need to link with the pthread library using `-pthread`.)
     ```bash
     gcc sleeping_barber.c -o sleeping_barber -pthread
     ```
   - **Execution:**  
     Run the executable while supplying two arguments: the number of chairs and the number of customers.
     ```bash
     ./sleeping_barber <number_of_chairs> <number_of_customers>
     ```

### Compilation and Execution

- **Compilation:**
  ```bash
  gcc sleeping_barber.c -o sleeping_barber -pthread
  ```
- **Execution:**
  ```bash
  ./sleeping_barber <number_of_chairs> <number_of_customers>
  ```
  For example:
  ```bash
  ./sleeping_barber 5 10
  ```
