#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

// Structure for shared data between processes
typedef struct {
    int waitingCustomers;  // Number of customers waiting
    int chairs;            // Total number of waiting chairs
    sem_t mutex;           // Mutex for protecting shared variables
    sem_t customerSem;     // Semaphore to indicate waiting customers
    sem_t barberSem;       // Semaphore to signal the barber is ready
} SharedData;

// Function prototypes
void barber(SharedData *shared);
void customer(SharedData *shared, int customerID);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_chairs> <number_of_customers>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int chairs = atoi(argv[1]);
    int totalCustomers = atoi(argv[2]);
    if (chairs < 0 || totalCustomers < 0) {
        fprintf(stderr, "Number of chairs and customers must be non-negative integers.\n");
        exit(EXIT_FAILURE);
    }
    
    // Allocate shared memory for the SharedData structure
    SharedData *shared = mmap(NULL, sizeof(SharedData), 
                              PROT_READ | PROT_WRITE, 
                              MAP_SHARED | MAP_ANONYMOUS, 
                              -1, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    // Initialize shared data
    shared->waitingCustomers = 0;
    shared->chairs = chairs;
    
    // Initialize semaphores (shared among processes)
    if (sem_init(&shared->mutex, 1, 1) == -1 ||
        sem_init(&shared->customerSem, 1, 0) == -1 ||
        sem_init(&shared->barberSem, 1, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    
    // Fork the barber process
    pid_t barberPID = fork();
    if (barberPID < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (barberPID == 0) {
        // Child process becomes the barber
        barber(shared);
        exit(EXIT_SUCCESS);
    }
    
    // Main process now forks customer processes
    for (int i = 0; i < totalCustomers; i++) {
        pid_t customerPID = fork();
        if (customerPID < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (customerPID == 0) {
            // In customer process: simulate arrival time with a random delay
            srand(time(NULL) ^ (getpid()<<16));
            usleep((rand() % 500000)); // up to 0.5 sec delay
            customer(shared, i+1);
            exit(EXIT_SUCCESS);
        }
    }
    
    // Wait for all customer processes to finish
    for (int i = 0; i < totalCustomers; i++) {
        wait(NULL);
    }
    
    // After all customers are served, terminate the barber process.
    // (In this simplified simulation, we forcefully terminate the barber.)
    kill(barberPID, SIGTERM);
    wait(NULL);
    
    // Clean up semaphores and shared memory
    sem_destroy(&shared->mutex);
    sem_destroy(&shared->customerSem);
    sem_destroy(&shared->barberSem);
    munmap(shared, sizeof(SharedData));
    
    printf("Simulation complete.\n");
    return EXIT_SUCCESS;
}

/**
 * barber - Function executed by the barber process.
 * The barber waits for customers and serves them one at a time.
 */
void barber(SharedData *shared) {
    printf("Barber: Starting work. Waiting for customers...\n");
    while (1) {
        // Wait for a customer to signal (sleep if no customers)
        sem_wait(&shared->customerSem);
        
        // Acquire the mutex to update waiting count
        sem_wait(&shared->mutex);
        shared->waitingCustomers--; // Take one customer for a haircut
        printf("Barber: Starting haircut. Waiting customers: %d\n", shared->waitingCustomers);
        // Signal the customer that the barber is ready
        sem_post(&shared->barberSem);
        sem_post(&shared->mutex);
        
        // Simulate cutting hair
        sleep(1); // haircut takes 1 second (adjust as needed)
        printf("Barber: Finished haircut.\n");
    }
}

/**
 * customer - Function executed by each customer process.
 * A customer checks for an available waiting chair and either waits or leaves.
 */
void customer(SharedData *shared, int customerID) {
    printf("Customer %d: Arrived at the barbershop.\n", customerID);
    
    sem_wait(&shared->mutex);
    if (shared->waitingCustomers < shared->chairs) {
        // There is a free waiting chair: sit and wait
        shared->waitingCustomers++;
        printf("Customer %d: Sitting in waiting area. Waiting customers: %d\n", customerID, shared->waitingCustomers);
        sem_post(&shared->customerSem); // Signal that a new customer is waiting
        sem_post(&shared->mutex);
        
        // Wait for the barber to be ready
        sem_wait(&shared->barberSem);
        // Get a haircut
        printf("Customer %d: Getting a haircut.\n", customerID);
        // After haircut, simulate payment (e.g., print message)
        printf("Customer %d: Haircut done, paying at the cashier.\n", customerID);
    } else {
        // No waiting chairs available; leave the shop
        sem_post(&shared->mutex);
        printf("Customer %d: No available chairs. Leaving the shop.\n", customerID);
    }
}
