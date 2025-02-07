#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

// Maximum length for an order description.
#define MAX_DESC_LEN 256

// Structure representing a single order.
typedef struct {
    int order_id;
    int table_number;
    char description[MAX_DESC_LEN];
    int prep_time; // Preparation time in seconds.
} Order;

// Node for the work queue (linked list).
typedef struct OrderNode {
    Order order;
    struct OrderNode *next;
} OrderNode;

// The work queue structure.
typedef struct {
    OrderNode *head;
    OrderNode *tail;
    int count;
} OrderQueue;

// Global shared data.
OrderQueue orderQueue = {NULL, NULL, 0};
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueCond = PTHREAD_COND_INITIALIZER;

int totalProcessed = 0; // Total orders processed by chefs.

// For reading orders from the input file.
FILE *order_fp = NULL;
pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

// Simulation control.
volatile int shutdownFlag = 0;  // Set to 1 when simulation time is over.
time_t simulationEndTime;       // Global simulation end time.

// ------------- Work Queue Functions ----------------

// Enqueue an order into the shared work queue.
void enqueueOrder(Order order) {
    OrderNode *node = malloc(sizeof(OrderNode));
    if (!node) {
        perror("malloc failed in enqueueOrder");
        exit(EXIT_FAILURE);
    }
    node->order = order;
    node->next = NULL;
    
    pthread_mutex_lock(&queueMutex);
    if (orderQueue.tail == NULL) {
        orderQueue.head = orderQueue.tail = node;
    } else {
        orderQueue.tail->next = node;
        orderQueue.tail = node;
    }
    orderQueue.count++;
    // Signal one waiting chef thread.
    pthread_cond_signal(&queueCond);
    pthread_mutex_unlock(&queueMutex);
}

// Dequeue an order from the shared work queue.
// Returns 1 if an order was dequeued, or 0 if the queue is empty and shutdown is in effect.
int dequeueOrder(Order *order) {
    pthread_mutex_lock(&queueMutex);
    // Use a loop to wait for a task or shutdown.
    while (orderQueue.count == 0 && !shutdownFlag) {
        // Wait with a timeout (optional; here we wait indefinitely).
        pthread_cond_wait(&queueCond, &queueMutex);
    }
    if (orderQueue.count == 0 && shutdownFlag) {
        pthread_mutex_unlock(&queueMutex);
        return 0;
    }
    // Dequeue the order.
    OrderNode *node = orderQueue.head;
    *order = node->order;
    orderQueue.head = node->next;
    if (orderQueue.head == NULL)
        orderQueue.tail = NULL;
    orderQueue.count--;
    free(node);
    pthread_mutex_unlock(&queueMutex);
    return 1;
}

// ------------- Thread Functions -----------------

// Customer thread function: reads orders from the file and enqueues them.
void *customerThread(void *arg) {
    int thread_id = *(int *)arg;
    free(arg);
    char line[512];
    while (time(NULL) < simulationEndTime) {
        pthread_mutex_lock(&fileMutex);
        if (fgets(line, sizeof(line), order_fp) == NULL) {
            // End of file reached: break out of loop.
            pthread_mutex_unlock(&fileMutex);
            break;
        }
        pthread_mutex_unlock(&fileMutex);

        // Remove any trailing newline.
        line[strcspn(line, "\n")] = '\0';

        // Parse the line: expected format: ORDER_ID,TableNumber,OrderDescription,PreparationTime
        Order order;
        char *token = strtok(line, ",");
        if (!token) continue;
        order.order_id = atoi(token);
        
        token = strtok(NULL, ",");
        if (!token) continue;
        order.table_number = atoi(token);
        
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(order.description, token, MAX_DESC_LEN - 1);
        order.description[MAX_DESC_LEN - 1] = '\0';
        
        token = strtok(NULL, ",");
        if (!token) continue;
        order.prep_time = atoi(token);

        // Enqueue the order.
        enqueueOrder(order);
        printf("[Customer %d] Enqueued Order %d for Table %d: %s (Prep Time: %d sec)\n",
               thread_id, order.order_id, order.table_number, order.description, order.prep_time);

        // Sleep for a short random duration (e.g., 0.5 to 1 second) before placing the next order.
        usleep((500 + rand() % 500) * 1000);
    }
    printf("[Customer %d] Finished enqueuing orders.\n", thread_id);
    return NULL;
}

// Chef thread function: dequeues and processes orders.
void *chefThread(void *arg) {
    int thread_id = *(int *)arg;
    free(arg);
    Order order;
    while (1) {
        if (!dequeueOrder(&order))
            break; // Shutdown and empty queue.
        printf("[Chef %d] Processing Order %d: %s (Table %d, Prep Time: %d sec)\n",
               thread_id, order.order_id, order.description, order.table_number, order.prep_time);
        sleep(order.prep_time);
        pthread_mutex_lock(&queueMutex);
        totalProcessed++;
        pthread_mutex_unlock(&queueMutex);
        printf("[Chef %d] Completed Order %d. Total processed: %d\n",
               thread_id, order.order_id, totalProcessed);
    }
    printf("[Chef %d] No more orders. Exiting.\n", thread_id);
    return NULL;
}

// Reporter thread function: periodically reports the system status.
void *reporterThread(void *arg) {
    (void)arg; // Unused parameter.
    while (1) {
        sleep(2); // Report every 2 seconds.
        pthread_mutex_lock(&queueMutex);
        int waiting = orderQueue.count;
        int processed = totalProcessed;
        pthread_mutex_unlock(&queueMutex);
        printf("[Reporter] Orders waiting: %d, Total processed: %d\n", waiting, processed);
        // If shutdown is signaled and the queue is empty, exit the reporter.
        if (shutdownFlag && waiting == 0)
            break;
    }
    printf("[Reporter] Final status reported. Exiting.\n");
    return NULL;
}

// ------------- Main Function -----------------

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <num_customers> <num_chefs> <input_file> <simulation_duration>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int numCustomers = atoi(argv[1]);
    int numChefs = atoi(argv[2]);
    char *inputFile = argv[3];
    int simDuration = atoi(argv[4]);
    if (numCustomers <= 0 || numChefs <= 0 || simDuration <= 0) {
        fprintf(stderr, "All numeric arguments must be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    // Open the input file for reading orders.
    order_fp = fopen(inputFile, "r");
    if (!order_fp) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Seed the random number generator.
    srand(time(NULL));

    // Set simulation end time.
    simulationEndTime = time(NULL) + simDuration;

    // Create arrays for thread IDs.
    pthread_t *customerThreads = malloc(sizeof(pthread_t) * numCustomers);
    pthread_t *chefThreads = malloc(sizeof(pthread_t) * numChefs);
    pthread_t reporterThreadId;

    // Create the reporter thread.
    if (pthread_create(&reporterThreadId, NULL, reporterThread, NULL) != 0) {
        perror("pthread_create reporter failed");
        exit(EXIT_FAILURE);
    }

    // Create customer threads.
    for (int i = 0; i < numCustomers; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&customerThreads[i], NULL, customerThread, id) != 0) {
            perror("pthread_create customer failed");
            exit(EXIT_FAILURE);
        }
    }

    // Create chef threads.
    for (int i = 0; i < numChefs; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&chefThreads[i], NULL, chefThread, id) != 0) {
            perror("pthread_create chef failed");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for the simulation duration.
    sleep(simDuration);
    // Signal shutdown for customer threads.
    shutdownFlag = 1;
    // Close the input file so that customer threads reading from it will stop.
    pthread_mutex_lock(&fileMutex);
    fclose(order_fp);
    order_fp = NULL;
    pthread_mutex_unlock(&fileMutex);

    // Wait for customer threads to finish.
    for (int i = 0; i < numCustomers; i++) {
        pthread_join(customerThreads[i], NULL);
    }
    free(customerThreads);

    // At this point, no new orders are being enqueued.
    // Broadcast to wake any chef threads waiting on an empty queue.
    pthread_mutex_lock(&queueMutex);
    pthread_cond_broadcast(&queueCond);
    pthread_mutex_unlock(&queueMutex);

    // Wait for chef threads to finish processing remaining orders.
    for (int i = 0; i < numChefs; i++) {
        pthread_join(chefThreads[i], NULL);
    }
    free(chefThreads);

    // Wait for reporter thread to finish.
    pthread_join(reporterThreadId, NULL);

    // Clean up mutex and condition variable.
    pthread_mutex_destroy(&queueMutex);
    pthread_mutex_destroy(&fileMutex);
    pthread_cond_destroy(&queueCond);

    printf("Simulation complete. Total orders processed: %d\n", totalProcessed);
    return EXIT_SUCCESS;
}
