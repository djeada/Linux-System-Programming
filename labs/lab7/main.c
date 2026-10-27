/* C11/POSIX: reusable workers and a bounded, explicitly closed queue. */
#include "../common/bounded_queue.h"

#define MAX_DESC_LEN 256
#define QUEUE_CAPACITY 32

typedef struct {
  int task_id;
  char description[MAX_DESC_LEN];
  int duration;
} Task;
typedef struct {
  int id;
  LabQueue *queue;
} Worker;

static void *worker_thread(void *context) {
  Worker *worker = context;
  Task task;
  while (lab_queue_pop(worker->queue, &task)) {
    printf("[Worker %d] Processing Task %d: %s (Duration: %d sec)\n",
           worker->id, task.task_id, task.description, task.duration);
    lab_simulate_ms((long)task.duration * 1000);
    printf("[Worker %d] Completed Task %d\n", worker->id, task.task_id);
  }
  printf("[Worker %d] Shutting down.\n", worker->id);
  return NULL;
}

static bool parse_task(char *line, Task *task) {
  char *fields[3];
  if (!lab_fields(line, fields, 3) || strlen(fields[1]) >= MAX_DESC_LEN ||
      !lab_parse_int(fields[0], INT_MIN, &task->task_id) ||
      !lab_parse_int(fields[2], 0, &task->duration))
    return false;
  memcpy(task->description, fields[1], strlen(fields[1]) + 1);
  return true;
}

int main(int argc, char *argv[]) {
  int count;
  if (argc != 3 || !lab_parse_int(argv[1], 1, &count)) {
    fprintf(stderr, "Usage: %s <positive_workers> <tasks_file>\n", argv[0]);
    return EXIT_FAILURE;
  }
  FILE *file = fopen(argv[2], "r");
  if (!file) {
    perror("open tasks");
    return EXIT_FAILURE;
  }
  pthread_t *threads = calloc((size_t)count, sizeof(*threads));
  Worker *workers = calloc((size_t)count, sizeof(*workers));
  if (!threads || !workers) {
    perror("calloc workers");
    fclose(file);
    free(threads);
    free(workers);
    return EXIT_FAILURE;
  }
  LabQueue queue;
  if (!lab_queue_init(&queue, QUEUE_CAPACITY, sizeof(Task))) {
    fclose(file);
    free(threads);
    free(workers);
    return EXIT_FAILURE;
  }
  int started = 0;
  bool success = true;
  for (; started < count; ++started) {
    workers[started] = (Worker){.id = started + 1, .queue = &queue};
    int error = pthread_create(&threads[started], NULL, worker_thread,
                               &workers[started]);
    if (lab_thread_error(error, "pthread_create") != 0) {
      success = false;
      break;
    }
  }
  char *line = NULL;
  size_t capacity = 0;
  while (success && getline(&line, &capacity, file) >= 0) {
    if (!*lab_trim(line))
      continue;
    Task task;
    if (!parse_task(line, &task)) {
      fprintf(stderr, "Skipping malformed task\n");
      continue;
    }
    if (!lab_queue_push(&queue, &task, NULL)) {
      success = false;
      break;
    }
  }
  if (ferror(file)) {
    perror("read tasks");
    success = false;
  }
  free(line);
  fclose(file);
  lab_queue_close(&queue);
  for (int id = 0; id < started; ++id)
    lab_thread_check(pthread_join(threads[id], NULL), "pthread_join");
  lab_queue_destroy(&queue);
  free(workers);
  free(threads);
  if (success)
    puts("All tasks processed. Exiting.");
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
