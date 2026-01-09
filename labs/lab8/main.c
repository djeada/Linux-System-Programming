#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_DESC_LEN 256
#define LINE_BUFFER 512
#define REPORT_INTERVAL_SEC 2

typedef struct {
  int order_id;
  int table_number;
  char description[MAX_DESC_LEN];
  int prep_time;
} Order;

typedef struct OrderNode {
  Order order;
  struct OrderNode *next;
} OrderNode;

typedef struct {
  OrderNode *head;
  OrderNode *tail;
  int count;
} OrderQueue;

typedef struct {
  Order *orders;
  size_t total;
  size_t next_index;
  pthread_mutex_t mutex;
} OrderPool;

typedef struct {
  OrderQueue queue;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int total_processed;
  bool producers_done;
} SharedState;

typedef struct {
  int id;
  time_t end_time;
  OrderPool *pool;
  SharedState *shared;
} CustomerArgs;

typedef struct {
  int id;
  SharedState *shared;
} ChefArgs;

static bool parse_order_line(const char *line, Order *order_out);
static int parse_int_field(const char *text, int *value_out);
static char *trim_whitespace(char *text);
static Order *load_orders(const char *path, size_t *count_out);

static void enqueue_order(SharedState *shared, const Order *order);
static bool dequeue_order(SharedState *shared, Order *order_out);
static void drain_queue(SharedState *shared);

static void *customer_thread(void *arg);
static void *chef_thread(void *arg);
static void *reporter_thread(void *arg);

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr,
            "Usage: %s <num_customers> <num_chefs> <input_file> "
            "<simulation_duration>\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  int num_customers = 0;
  int num_chefs = 0;
  int sim_duration = 0;

  if (parse_int_field(argv[1], &num_customers) != 0 || num_customers <= 0 ||
      parse_int_field(argv[2], &num_chefs) != 0 || num_chefs <= 0 ||
      parse_int_field(argv[4], &sim_duration) != 0 || sim_duration <= 0) {
    fprintf(stderr, "All numeric arguments must be positive integers.\n");
    return EXIT_FAILURE;
  }

  size_t order_count = 0;
  Order *orders = load_orders(argv[3], &order_count);
  if (!orders || order_count == 0) {
    fprintf(stderr, "No valid orders found in input file.\n");
    free(orders);
    return EXIT_FAILURE;
  }

  OrderPool pool = {
      .orders = orders,
      .total = order_count,
      .next_index = 0,
      .mutex = PTHREAD_MUTEX_INITIALIZER,
  };

  SharedState shared = {
      .queue = {NULL, NULL, 0},
      .mutex = PTHREAD_MUTEX_INITIALIZER,
      .cond = PTHREAD_COND_INITIALIZER,
      .total_processed = 0,
      .producers_done = false,
  };

  time_t end_time = time(NULL) + sim_duration;

  pthread_t *customer_threads =
      calloc((size_t)num_customers, sizeof(*customer_threads));
  pthread_t *chef_threads = calloc((size_t)num_chefs, sizeof(*chef_threads));
  if (!customer_threads || !chef_threads) {
    perror("calloc");
    free(orders);
    free(customer_threads);
    free(chef_threads);
    return EXIT_FAILURE;
  }

  pthread_t reporter_id;
  if (pthread_create(&reporter_id, NULL, reporter_thread, &shared) != 0) {
    perror("pthread_create reporter");
    free(orders);
    free(customer_threads);
    free(chef_threads);
    return EXIT_FAILURE;
  }

  int created_customers = 0;
  int created_chefs = 0;
  bool start_ok = true;

  for (int i = 0; i < num_customers; i++) {
    CustomerArgs *args = malloc(sizeof(*args));
    if (!args) {
      perror("malloc customer args");
      start_ok = false;
      break;
    }
    args->id = i + 1;
    args->end_time = end_time;
    args->pool = &pool;
    args->shared = &shared;
    if (pthread_create(&customer_threads[i], NULL, customer_thread, args) !=
        0) {
      perror("pthread_create customer");
      free(args);
      start_ok = false;
      break;
    }
    created_customers++;
  }

  for (int i = 0; i < num_chefs && start_ok; i++) {
    ChefArgs *args = malloc(sizeof(*args));
    if (!args) {
      perror("malloc chef args");
      start_ok = false;
      break;
    }
    args->id = i + 1;
    args->shared = &shared;
    if (pthread_create(&chef_threads[i], NULL, chef_thread, args) != 0) {
      perror("pthread_create chef");
      free(args);
      start_ok = false;
      break;
    }
    created_chefs++;
  }

  if (!start_ok) {
    pthread_mutex_lock(&shared.mutex);
    shared.producers_done = true;
    pthread_cond_broadcast(&shared.cond);
    pthread_mutex_unlock(&shared.mutex);

    for (int i = 0; i < created_customers; i++) {
      pthread_join(customer_threads[i], NULL);
    }
    for (int i = 0; i < created_chefs; i++) {
      pthread_join(chef_threads[i], NULL);
    }

    drain_queue(&shared);
    pthread_join(reporter_id, NULL);

    pthread_mutex_destroy(&shared.mutex);
    pthread_cond_destroy(&shared.cond);
    pthread_mutex_destroy(&pool.mutex);

    free(orders);
    free(customer_threads);
    free(chef_threads);
    return EXIT_FAILURE;
  }

  for (int i = 0; i < created_customers; i++) {
    pthread_join(customer_threads[i], NULL);
  }

  pthread_mutex_lock(&shared.mutex);
  shared.producers_done = true;
  pthread_cond_broadcast(&shared.cond);
  pthread_mutex_unlock(&shared.mutex);

  for (int i = 0; i < created_chefs; i++) {
    pthread_join(chef_threads[i], NULL);
  }

  pthread_join(reporter_id, NULL);

  pthread_mutex_destroy(&shared.mutex);
  pthread_cond_destroy(&shared.cond);
  pthread_mutex_destroy(&pool.mutex);

  free(orders);
  free(customer_threads);
  free(chef_threads);

  printf("Simulation complete. Total orders processed: %d\n",
         shared.total_processed);
  return EXIT_SUCCESS;
}

static void *customer_thread(void *arg) {
  CustomerArgs *args = arg;
  int thread_id = args->id;
  OrderPool *pool = args->pool;
  SharedState *shared = args->shared;
  time_t end_time = args->end_time;
  free(args);

  unsigned int seed = (unsigned int)(time(NULL) ^ (thread_id << 16));

  while (time(NULL) < end_time) {
    pthread_mutex_lock(&pool->mutex);
    if (pool->next_index >= pool->total) {
      pthread_mutex_unlock(&pool->mutex);
      break;
    }
    Order order = pool->orders[pool->next_index++];
    pthread_mutex_unlock(&pool->mutex);

    enqueue_order(shared, &order);
    printf("[Customer %d] Enqueued Order %d for Table %d: %s (Prep Time: %d "
           "sec)\n",
           thread_id, order.order_id, order.table_number, order.description,
           order.prep_time);

    usleep((useconds_t)((500 + rand_r(&seed) % 500) * 1000));
  }

  printf("[Customer %d] Finished enqueuing orders.\n", thread_id);
  return NULL;
}

static void *chef_thread(void *arg) {
  ChefArgs *args = arg;
  int thread_id = args->id;
  SharedState *shared = args->shared;
  free(args);

  Order order;
  while (dequeue_order(shared, &order)) {
    printf("[Chef %d] Processing Order %d: %s (Table %d, Prep Time: %d sec)\n",
           thread_id, order.order_id, order.description, order.table_number,
           order.prep_time);
    sleep(order.prep_time);

    pthread_mutex_lock(&shared->mutex);
    shared->total_processed++;
    pthread_mutex_unlock(&shared->mutex);

    printf("[Chef %d] Completed Order %d. Total processed: %d\n", thread_id,
           order.order_id, shared->total_processed);
  }

  printf("[Chef %d] No more orders. Exiting.\n", thread_id);
  return NULL;
}

static void *reporter_thread(void *arg) {
  SharedState *shared = arg;

  while (1) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += REPORT_INTERVAL_SEC;

    pthread_mutex_lock(&shared->mutex);
    pthread_cond_timedwait(&shared->cond, &shared->mutex, &ts);
    int waiting = shared->queue.count;
    int processed = shared->total_processed;
    bool done = shared->producers_done;
    pthread_mutex_unlock(&shared->mutex);

    printf("[Reporter] Orders waiting: %d, Total processed: %d\n", waiting,
           processed);
    if (done && waiting == 0) {
      break;
    }
  }

  printf("[Reporter] Final status reported. Exiting.\n");
  return NULL;
}

static void enqueue_order(SharedState *shared, const Order *order) {
  OrderNode *node = malloc(sizeof(*node));
  if (!node) {
    perror("malloc order node");
    exit(EXIT_FAILURE);
  }
  node->order = *order;
  node->next = NULL;

  pthread_mutex_lock(&shared->mutex);
  if (shared->queue.tail) {
    shared->queue.tail->next = node;
  } else {
    shared->queue.head = node;
  }
  shared->queue.tail = node;
  shared->queue.count++;
  pthread_cond_signal(&shared->cond);
  pthread_mutex_unlock(&shared->mutex);
}

static bool dequeue_order(SharedState *shared, Order *order_out) {
  pthread_mutex_lock(&shared->mutex);
  while (shared->queue.count == 0 && !shared->producers_done) {
    pthread_cond_wait(&shared->cond, &shared->mutex);
  }
  if (shared->queue.count == 0 && shared->producers_done) {
    pthread_mutex_unlock(&shared->mutex);
    return false;
  }

  OrderNode *node = shared->queue.head;
  *order_out = node->order;
  shared->queue.head = node->next;
  if (!shared->queue.head) {
    shared->queue.tail = NULL;
  }
  shared->queue.count--;
  pthread_mutex_unlock(&shared->mutex);

  free(node);
  return true;
}

static void drain_queue(SharedState *shared) {
  pthread_mutex_lock(&shared->mutex);
  OrderNode *node = shared->queue.head;
  while (node) {
    OrderNode *next = node->next;
    free(node);
    node = next;
  }
  shared->queue.head = NULL;
  shared->queue.tail = NULL;
  shared->queue.count = 0;
  pthread_mutex_unlock(&shared->mutex);
}

static Order *load_orders(const char *path, size_t *count_out) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror("fopen orders file");
    return NULL;
  }

  size_t capacity = 32;
  size_t count = 0;
  Order *orders = calloc(capacity, sizeof(*orders));
  if (!orders) {
    perror("calloc orders");
    fclose(fp);
    return NULL;
  }

  char line[LINE_BUFFER];
  while (fgets(line, sizeof(line), fp)) {
    line[strcspn(line, "\r\n")] = '\0';
    if (line[0] == '\0') {
      continue;
    }

    Order order;
    if (!parse_order_line(line, &order)) {
      continue;
    }

    if (count == capacity) {
      capacity *= 2;
      Order *grown = realloc(orders, capacity * sizeof(*orders));
      if (!grown) {
        perror("realloc orders");
        free(orders);
        fclose(fp);
        return NULL;
      }
      orders = grown;
    }

    orders[count++] = order;
  }

  fclose(fp);
  *count_out = count;
  return orders;
}

static bool parse_order_line(const char *line, Order *order_out) {
  char buffer[LINE_BUFFER];
  strncpy(buffer, line, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';

  char *saveptr = NULL;
  char *token = strtok_r(buffer, ",", &saveptr);
  if (!token) {
    return false;
  }
  token = trim_whitespace(token);
  if (parse_int_field(token, &order_out->order_id) != 0) {
    return false;
  }

  token = strtok_r(NULL, ",", &saveptr);
  if (!token) {
    return false;
  }
  token = trim_whitespace(token);
  if (parse_int_field(token, &order_out->table_number) != 0) {
    return false;
  }

  token = strtok_r(NULL, ",", &saveptr);
  if (!token) {
    return false;
  }
  token = trim_whitespace(token);
  strncpy(order_out->description, token, sizeof(order_out->description) - 1);
  order_out->description[sizeof(order_out->description) - 1] = '\0';

  token = strtok_r(NULL, ",", &saveptr);
  if (!token) {
    return false;
  }
  token = trim_whitespace(token);
  if (parse_int_field(token, &order_out->prep_time) != 0 ||
      order_out->prep_time < 0) {
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
