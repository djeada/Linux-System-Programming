**Task: Restaurant Order Processing Simulation**

### **Objective**

Design and implement a concurrent simulation of a restaurant’s order processing system. This system will involve multiple types of threads that interact via a shared, thread-safe order queue. You will gain hands-on experience with:

- Creating and managing multiple threads (producers, consumers, and a reporter)
- Synchronizing threads using mutexes and condition variables (with timed waits)
- Dynamically enqueuing and dequeuing tasks (orders) in a shared work queue
- Periodically reporting system status while processing is underway

### **Task Description**

1. **Roles and Responsibilities:**

   - **Customer Threads (Producers):**
     - These threads simulate customers placing orders.
     - Orders are read from an input file (see “Input File Format” below) where each line describes an order (for example, order ID, table number, and order details).
     - Each customer thread enqueues one or more orders into the shared work queue.
   
   - **Chef Threads (Consumers):**
     - These threads represent chefs who process orders.
     - A chef thread continuously dequeues orders from the shared order queue.
     - When processing an order, a chef prints the order details and simulates preparation by sleeping for a specified duration (provided in the order).
     - Chefs update a shared statistic (such as the total number of orders processed) after completing each order.

   - **Reporter Thread:**
     - A dedicated reporter thread periodically (e.g., every few seconds) prints the current status of the system.
     - The status should include the number of orders waiting in the queue and the total number of orders processed so far.
     - This thread must use timed waits to trigger status reports without interfering with order processing.

2. **Shared Work Queue:**

   - Implement the order queue using a dynamic data structure (for example, a linked list or circular buffer).
   - The queue must be protected by a mutex so that concurrent access (from customer and chef threads) does not lead to race conditions.
   - A condition variable is used to signal chef threads when new orders are enqueued.
   - Use a timed wait mechanism for chef threads, if desired, to allow them to periodically check for a graceful shutdown signal (see Shutdown section below).

3. **Input File Format:**

   - The program will read orders from a text file specified by a command-line argument.
   - Each line in the file represents a single order and is formatted as follows:
     ```
     ORDER_ID,TableNumber,OrderDescription,PreparationTime
     ```
     For example:
     ```
     101,5,Pizza Margherita,3
     102,2,Spaghetti Carbonara,2
     103,8,Caesar Salad,1
     ```
   - Ensure that your program correctly parses each line into an order structure.

4. **Command-Line Arguments:**

   Your program should accept the following arguments:
   - The number of customer threads (producers)
   - The number of chef threads (consumers)
   - The path to the input file containing orders
   - The total simulation duration (in seconds) after which no new orders will be enqueued and the system will begin a graceful shutdown

   **Example:**
   ```bash
   ./restaurant 4 3 orders.txt 30
   ```
   This command starts 4 customer threads, 3 chef threads, reads orders from `orders.txt`, and runs the simulation for 30 seconds.

5. **Shutdown and Cleanup:**

   - After the specified simulation duration, the system should signal all threads to finish processing.
   - Customer threads should stop enqueuing new orders.
   - Chef threads should finish processing any remaining orders in the queue and then exit gracefully.
   - The reporter thread should perform a final status report before terminating.
   - All dynamically allocated resources (such as memory for the work queue, mutexes, and condition variables) must be properly cleaned up before the program exits.

6. **Synchronization and Concurrency:**

   - Use mutexes to protect shared resources (order queue and statistics).
   - Employ condition variables to manage signaling between customer and chef threads.
   - Incorporate timed waits (e.g., `pthread_cond_timedwait`) to allow threads to periodically check for shutdown signals or to trigger periodic reports in the reporter thread.
   - Design your solution to avoid race conditions, deadlocks, and starvation.

### **Additional Considerations**

- **Robustness:**  
  Ensure your program gracefully handles errors such as file I/O issues, thread creation failures, or malformed input lines.

- **Modular Design:**  
  Break down your solution into clearly defined functions:
  - Functions for enqueuing and dequeuing orders
  - A customer thread function that reads orders from the input file and enqueues them
  - A chef thread function that dequeues and processes orders
  - A reporter thread function that prints status updates at regular intervals

- **Testing:**  
  Test your program with various input files, different numbers of customer and chef threads, and multiple simulation durations to ensure that your system behaves correctly under different loads and shutdown conditions.

