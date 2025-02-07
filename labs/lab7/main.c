#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

// Maximum length for task description strings.
#define MAX_DESC_LEN 256

// Structure representing a single task.
typedef struct Task {
    int task_id;
    char description[MAX_DESC_LEN];
    int duration;  // Duration in seconds.
} Task;

// Node for linked list work queue.
typedef struct TaskNode {
    Task task;
    struct TaskNode *next;
} TaskNode;

// Work queue structure.
typedef struct {
    TaskNode *head;
    TaskNode *tail;
    int count;
} WorkQueue;

// Global work queue, mutex, condition variable, and shutdown flag.
WorkQueue workQueue = {NULL, NULL, 0};
pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueCond = PTHREAD_COND_INITIALIZER;
int shutdownFlag = 0; // When set to 1, no more tasks will be added.

// Function to enqueue a task.
void enqueueTask(Task task) {
    TaskNode *node = malloc(sizeof(TaskNode));
    if (!node) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    node->task = task;
    node->next = NULL;

    pthread_mutex_lock(&queueMutex);
    if (workQueue.tail == NULL) {
        workQueue.head = workQueue.tail = node;
    } else {
        workQueue.tail->next = node;
        workQueue.tail = node;
    }
    workQueue.count++;
    // Signal one waiting worker that a new task is available.
    pthread_cond_signal(&queueCond);
    pthread_mutex_unlock(&queueMutex);
}

// Function to dequeue a task.
// Returns 1 if a task was dequeued, 0 if the queue is empty.
int dequeueTask(Task *task) {
    pthread_mutex_lock(&queueMutex);
    while (workQueue.count == 0 && !shutdownFlag) {
        // Wait for tasks to be enqueued or shutdown to be signaled.
        pthread_cond_wait(&queueCond, &queueMutex);
    }
    if (workQueue.count == 0 && shutdownFlag) {
        // No tasks and shutdown is requested.
        pthread_mutex_unlock(&queueMutex);
        return 0;
    }
    // Remove the task from the head of the queue.
    TaskNode *node = workQueue.head;
    *task = node->task;
    workQueue.head = node->next;
    if (workQueue.head == NULL)
        workQueue.tail = NULL;
    workQueue.count--;
    free(node);
    pthread_mutex_unlock(&queueMutex);
    return 1;
}

// Worker thread function.
void *workerFunction(void *arg) {
    long thread_id = (long)arg;
    Task task;
    while (1) {
        // Try to dequeue a task. If no task is available and shutdownFlag is set, exit.
        if (!dequeueTask(&task))
            break;

        // Print task details and simulate work.
        printf("[Worker %ld] Processing Task %d: %s (Duration: %d sec)\n",
               thread_id, task.task_id, task.description, task.duration);
        sleep(task.duration);
        printf("[Worker %ld] Completed Task %d\n", thread_id, task.task_id);
    }
    printf("[Worker %ld] Shutting down.\n", thread_id);
    return NULL;
}

// Function to parse a task from a line.
// Expected format: TASK_ID,Task description,Duration_in_seconds
int parseTask(char *line, Task *task) {
    char *token;
    // Get TASK_ID.
    token = strtok(line, ",");
    if (!token)
        return -1;
    task->task_id = atoi(token);

    // Get Task description.
    token = strtok(NULL, ",");
    if (!token)
        return -1;
    strncpy(task->description, token, MAX_DESC_LEN-1);
    task->description[MAX_DESC_LEN-1] = '\0';

    // Get Duration.
    token = strtok(NULL, ",");
    if (!token)
        return -1;
    task->duration = atoi(token);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_worker_threads> <input_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int numWorkers = atoi(argv[1]);
    if (numWorkers <= 0) {
        fprintf(stderr, "Number of worker threads must be a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    char *inputFile = argv[2];
    FILE *fp = fopen(inputFile, "r");
    if (!fp) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Create worker threads.
    pthread_t *workers = malloc(sizeof(pthread_t) * numWorkers);
    if (!workers) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < numWorkers; i++) {
        if (pthread_create(&workers[i], NULL, workerFunction, (void *)i) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    // Read tasks from the input file and enqueue them.
    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Remove newline at the end, if any.
        line[strcspn(line, "\n")] = '\0';
        Task task;
        if (parseTask(line, &task) != 0) {
            fprintf(stderr, "Error parsing task: %s\n", line);
            continue;
        }
        enqueueTask(task);
    }
    fclose(fp);

    // All tasks have been enqueued; signal shutdown.
    pthread_mutex_lock(&queueMutex);
    shutdownFlag = 1;
    pthread_cond_broadcast(&queueCond);
    pthread_mutex_unlock(&queueMutex);

    // Wait for all worker threads to finish.
    for (int i = 0; i < numWorkers; i++) {
        pthread_join(workers[i], NULL);
    }
    free(workers);

    // Clean up mutex and condition variable.
    pthread_mutex_destroy(&queueMutex);
    pthread_cond_destroy(&queueCond);

    printf("All tasks processed. Exiting.\n");
    return EXIT_SUCCESS;
}
