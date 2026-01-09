#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_DESC_LEN 256
#define LINE_BUFFER 512

typedef struct {
  int task_id;
  char description[MAX_DESC_LEN];
  int duration;
} Task;

typedef struct TaskNode {
  Task task;
  struct TaskNode *next;
} TaskNode;

typedef struct {
  TaskNode *head;
  TaskNode *tail;
  size_t count;
} WorkQueue;

static WorkQueue queue = {NULL, NULL, 0};
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;
static bool shutdown_flag = false;

static void enqueue_task(const Task *task);
static bool dequeue_task(Task *task);
static void signal_shutdown(void);
static bool parse_task_line(const char *line, Task *task_out);
static int parse_int_field(const char *text, int *value_out);
static char *trim_whitespace(char *text);
static void drain_queue(void);

static void *worker_thread(void *arg);

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <number_of_worker_threads> <input_file>\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  int num_workers = 0;
  if (parse_int_field(argv[1], &num_workers) != 0 || num_workers <= 0) {
    fprintf(stderr, "Number of worker threads must be a positive integer.\n");
    return EXIT_FAILURE;
  }

  FILE *fp = fopen(argv[2], "r");
  if (!fp) {
    perror("fopen");
    return EXIT_FAILURE;
  }

  pthread_t *workers = calloc((size_t)num_workers, sizeof(*workers));
  if (!workers) {
    perror("calloc");
    fclose(fp);
    return EXIT_FAILURE;
  }

  int created_workers = 0;
  for (int i = 0; i < num_workers; i++) {
    if (pthread_create(&workers[i], NULL, worker_thread, NULL) != 0) {
      perror("pthread_create");
      signal_shutdown();
      created_workers = i;
      fclose(fp);
      for (int j = 0; j < created_workers; j++) {
        pthread_join(workers[j], NULL);
      }
      free(workers);
      return EXIT_FAILURE;
    }
    created_workers++;
  }

  char line[LINE_BUFFER];
  while (fgets(line, sizeof(line), fp)) {
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] == '\0') {
      continue;
    }
    Task task;
    if (!parse_task_line(line, &task)) {
      fprintf(stderr, "Skipping malformed task: %s\n", line);
      continue;
    }
    enqueue_task(&task);
  }

  fclose(fp);
  signal_shutdown();

  for (int i = 0; i < created_workers; i++) {
    pthread_join(workers[i], NULL);
  }

  free(workers);
  drain_queue();
  pthread_mutex_destroy(&queue_mutex);
  pthread_cond_destroy(&queue_cond);

  printf("All tasks processed. Exiting.\n");
  return EXIT_SUCCESS;
}

static void enqueue_task(const Task *task) {
  TaskNode *node = malloc(sizeof(*node));
  if (!node) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  node->task = *task;
  node->next = NULL;

  pthread_mutex_lock(&queue_mutex);
  if (queue.tail) {
    queue.tail->next = node;
  } else {
    queue.head = node;
  }
  queue.tail = node;
  queue.count++;
  pthread_cond_signal(&queue_cond);
  pthread_mutex_unlock(&queue_mutex);
}

static bool dequeue_task(Task *task) {
  pthread_mutex_lock(&queue_mutex);
  while (queue.count == 0 && !shutdown_flag) {
    pthread_cond_wait(&queue_cond, &queue_mutex);
  }

  if (queue.count == 0 && shutdown_flag) {
    pthread_mutex_unlock(&queue_mutex);
    return false;
  }

  TaskNode *node = queue.head;
  *task = node->task;
  queue.head = node->next;
  if (!queue.head) {
    queue.tail = NULL;
  }
  queue.count--;
  pthread_mutex_unlock(&queue_mutex);

  free(node);
  return true;
}

static void signal_shutdown(void) {
  pthread_mutex_lock(&queue_mutex);
  shutdown_flag = true;
  pthread_cond_broadcast(&queue_cond);
  pthread_mutex_unlock(&queue_mutex);
}

static bool parse_task_line(const char *line, Task *task_out) {
  char buffer[LINE_BUFFER];
  strncpy(buffer, line, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  char *saveptr = NULL;
  char *token = strtok_r(buffer, ",", &saveptr);
  if (!token) {
    return false;
  }
  token = trim_whitespace(token);
  if (parse_int_field(token, &task_out->task_id) != 0) {
    return false;
  }

  token = strtok_r(NULL, ",", &saveptr);
  if (!token) {
    return false;
  }
  token = trim_whitespace(token);
  strncpy(task_out->description, token, sizeof(task_out->description) - 1);
  task_out->description[sizeof(task_out->description) - 1] = '\0';

  token = strtok_r(NULL, ",", &saveptr);
  if (!token) {
    return false;
  }
  token = trim_whitespace(token);
  if (parse_int_field(token, &task_out->duration) != 0 ||
      task_out->duration < 0) {
    return false;
  }

  return true;
}

static int parse_int_field(const char *text, int *value_out) {
  char *end = NULL;
  errno = 0;
  long value = strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value < INT_MIN ||
      value > INT_MAX) {
    return -1;
  }
  *value_out = (int)value;
  return 0;
}

static char *trim_whitespace(char *text) {
  while (*text && isspace((unsigned char)*text)) {
    text++;
  }

  char *end = text + strlen(text);
  while (end > text) {
    end--;
    if (!isspace((unsigned char)*end)) {
      break;
    }
    *end = '\0';
  }

  return text;
}

static void drain_queue(void) {
  pthread_mutex_lock(&queue_mutex);
  TaskNode *node = queue.head;
  while (node) {
    TaskNode *next = node->next;
    free(node);
    node = next;
  }
  queue.head = NULL;
  queue.tail = NULL;
  queue.count = 0;
  pthread_mutex_unlock(&queue_mutex);
}

static void *worker_thread(void *arg) {
  (void)arg;
  Task task;

  while (dequeue_task(&task)) {
    printf("[Worker %lu] Processing Task %d: %s (Duration: %d sec)\n",
           (unsigned long)pthread_self(), task.task_id, task.description,
           task.duration);
    sleep(task.duration);
    printf("[Worker %lu] Completed Task %d\n", (unsigned long)pthread_self(),
           task.task_id);
  }

  printf("[Worker %lu] Shutting down.\n", (unsigned long)pthread_self());
  return NULL;
}
