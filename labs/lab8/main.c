/* C11/POSIX: bounded streaming orders, monotonic deadlines, and final reporting
   after every chef has joined. Thread arguments stay owned by main. */
#include "../common/bounded_queue.h"

#define MAX_DESC_LEN 256
#define QUEUE_CAPACITY 32
#define REPORT_INTERVAL_SEC 2

typedef struct {
  int order_id, table_number, prep_time;
  char description[MAX_DESC_LEN];
} Order;
typedef struct {
  FILE *file;
  char *line;
  size_t capacity;
  bool failed;
  pthread_mutex_t mutex;
} OrderSource;
typedef struct {
  LabQueue queue;
  pthread_mutex_t mutex;
  pthread_cond_t changed;
  size_t processed;
  bool finished;
} SharedState;
typedef struct {
  int id;
  struct timespec deadline;
  OrderSource *source;
  SharedState *shared;
} CustomerArgs;
typedef struct {
  int id;
  SharedState *shared;
} ChefArgs;

static bool parse_order(char *line, Order *order) {
  char *fields[4];
  if (!lab_fields(line, fields, 4) || strlen(fields[2]) >= MAX_DESC_LEN ||
      !lab_parse_int(fields[0], INT_MIN, &order->order_id) ||
      !lab_parse_int(fields[1], INT_MIN, &order->table_number) ||
      !lab_parse_int(fields[3], 0, &order->prep_time))
    return false;
  memcpy(order->description, fields[2], strlen(fields[2]) + 1);
  return true;
}

static bool next_order(OrderSource *source, Order *order) {
  lab_thread_check(pthread_mutex_lock(&source->mutex), "lock order source");
  bool found = false;
  while (getline(&source->line, &source->capacity, source->file) >= 0) {
    if (!*lab_trim(source->line))
      continue;
    if (parse_order(source->line, order)) {
      found = true;
      break;
    }
    fprintf(stderr, "Skipping malformed order\n");
  }
  if (ferror(source->file)) {
    perror("read orders");
    source->failed = true;
  }
  lab_thread_check(pthread_mutex_unlock(&source->mutex), "unlock order source");
  return found;
}

static void *customer_thread(void *context) {
  CustomerArgs *args = context;
  while (lab_before(lab_now(), args->deadline)) {
    Order order;
    if (!next_order(args->source, &order))
      break;
    if (!lab_queue_push(&args->shared->queue, &order, &args->deadline))
      break;
    printf("[Customer %d] Enqueued Order %d for Table %d: %s\n", args->id,
           order.order_id, order.table_number, order.description);
    lab_simulate_ms(500);
  }
  printf("[Customer %d] Finished enqueuing orders.\n", args->id);
  return NULL;
}

static void *chef_thread(void *context) {
  ChefArgs *args = context;
  Order order;
  while (lab_queue_pop(&args->shared->queue, &order)) {
    printf("[Chef %d] Processing Order %d: %s\n", args->id, order.order_id,
           order.description);
    lab_simulate_ms((long)order.prep_time * 1000);
    lab_thread_check(pthread_mutex_lock(&args->shared->mutex),
                     "lock statistics");
    size_t completed = ++args->shared->processed;
    lab_thread_check(pthread_mutex_unlock(&args->shared->mutex),
                     "unlock statistics");
    printf("[Chef %d] Completed Order %d. Total processed: %zu\n", args->id,
           order.order_id, completed);
  }
  printf("[Chef %d] No more orders. Exiting.\n", args->id);
  return NULL;
}

static void *reporter_thread(void *context) {
  SharedState *shared = context;
  for (;;) {
    struct timespec deadline = lab_now();
    deadline.tv_sec += REPORT_INTERVAL_SEC;
    lab_thread_check(pthread_mutex_lock(&shared->mutex), "lock statistics");
    while (!shared->finished) {
      int code =
          pthread_cond_timedwait(&shared->changed, &shared->mutex, &deadline);
      if (code == ETIMEDOUT)
        break;
      lab_thread_check(code, "wait for reporting deadline");
    }
    bool finished = shared->finished;
    size_t completed = shared->processed;
    lab_thread_check(pthread_mutex_unlock(&shared->mutex), "unlock statistics");
    printf("[Reporter] Orders waiting: %zu, Total processed: %zu\n",
           lab_queue_count(&shared->queue), completed);
    if (finished)
      break;
  }
  puts("[Reporter] Final status reported. Exiting.");
  return NULL;
}

int main(int argc, char *argv[]) {
  int customers, chefs, duration;
  if (argc != 5 || !lab_parse_int(argv[1], 1, &customers) ||
      !lab_parse_int(argv[2], 1, &chefs) ||
      !lab_parse_int(argv[4], 1, &duration)) {
    fprintf(stderr,
            "Usage: %s <positive_customers> <positive_chefs> <orders_file> "
            "<positive_seconds>\n",
            argv[0]);
    return EXIT_FAILURE;
  }
  OrderSource source = {0};
  SharedState shared = {0};
  source.file = fopen(argv[3], "r");
  if (!source.file) {
    perror("open orders");
    return EXIT_FAILURE;
  }
  pthread_t *customer_ids = calloc((size_t)customers, sizeof(*customer_ids));
  pthread_t *chef_ids = calloc((size_t)chefs, sizeof(*chef_ids));
  CustomerArgs *customer_args =
      calloc((size_t)customers, sizeof(*customer_args));
  ChefArgs *chef_args = calloc((size_t)chefs, sizeof(*chef_args));
  bool success = false, queue_ready = false, source_ready = false,
       stats_ready = false, cond_ready = false;
  if (!customer_ids || !chef_ids || !customer_args || !chef_args) {
    perror("calloc threads");
    goto cleanup;
  }
  if (!lab_queue_init(&shared.queue, QUEUE_CAPACITY, sizeof(Order)))
    goto cleanup;
  queue_ready = true;
  if (lab_thread_error(pthread_mutex_init(&source.mutex, NULL),
                       "initialize source mutex"))
    goto cleanup;
  source_ready = true;
  if (lab_thread_error(pthread_mutex_init(&shared.mutex, NULL),
                       "initialize statistics mutex"))
    goto cleanup;
  stats_ready = true;
  pthread_condattr_t attr;
  if (lab_thread_error(pthread_condattr_init(&attr),
                       "initialize condition attributes"))
    goto cleanup;
  int code = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
  if (!code)
    code = pthread_cond_init(&shared.changed, &attr);
  pthread_condattr_destroy(&attr);
  if (lab_thread_error(code, "initialize reporter condition"))
    goto cleanup;
  cond_ready = true;

  pthread_t reporter;
  if (lab_thread_error(
          pthread_create(&reporter, NULL, reporter_thread, &shared),
          "start reporter"))
    goto cleanup;
  int started_chefs = 0, started_customers = 0;
  success = true;
  struct timespec deadline = lab_now();
  deadline.tv_sec += duration;
  /* Consumers must exist before a producer can block on a full queue. */
  for (; started_chefs < chefs; ++started_chefs) {
    chef_args[started_chefs] =
        (ChefArgs){.id = started_chefs + 1, .shared = &shared};
    code = pthread_create(&chef_ids[started_chefs], NULL, chef_thread,
                          &chef_args[started_chefs]);
    if (lab_thread_error(code, "start chef")) {
      success = false;
      break;
    }
  }
  for (; success && started_customers < customers; ++started_customers) {
    customer_args[started_customers] =
        (CustomerArgs){.id = started_customers + 1,
                       .deadline = deadline,
                       .source = &source,
                       .shared = &shared};
    code = pthread_create(&customer_ids[started_customers], NULL,
                          customer_thread, &customer_args[started_customers]);
    if (lab_thread_error(code, "start customer")) {
      success = false;
      break;
    }
  }
  if (!success)
    lab_queue_close(&shared.queue);
  for (int id = 0; id < started_customers; ++id)
    lab_thread_check(pthread_join(customer_ids[id], NULL), "join customer");
  lab_queue_close(&shared.queue);
  for (int id = 0; id < started_chefs; ++id)
    lab_thread_check(pthread_join(chef_ids[id], NULL), "join chef");
  lab_thread_check(pthread_mutex_lock(&shared.mutex), "lock statistics");
  shared.finished = true;
  lab_thread_check(pthread_cond_broadcast(&shared.changed), "wake reporter");
  lab_thread_check(pthread_mutex_unlock(&shared.mutex), "unlock statistics");
  lab_thread_check(pthread_join(reporter, NULL), "join reporter");
  success = success && !source.failed;
  if (success)
    printf("Simulation complete. Total orders processed: %zu\n",
           shared.processed);
cleanup:
  if (cond_ready)
    pthread_cond_destroy(&shared.changed);
  if (stats_ready)
    pthread_mutex_destroy(&shared.mutex);
  if (source_ready)
    pthread_mutex_destroy(&source.mutex);
  if (queue_ready)
    lab_queue_destroy(&shared.queue);
  fclose(source.file);
  free(source.line);
  free(customer_ids);
  free(chef_ids);
  free(customer_args);
  free(chef_args);
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
