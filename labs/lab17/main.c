#include "../common/io.h"
#include <stdatomic.h>

typedef struct {
  atomic_uint_fast64_t *counter;
  uint64_t iterations, completed;
} Worker;
static void *work(void *argument) {
  Worker *worker = argument;
  for (uint64_t i = 0; i < worker->iterations; ++i)
    atomic_fetch_add_explicit(worker->counter, 1, memory_order_relaxed);
  worker->completed =
      worker->iterations; /* Published to main by pthread_join. */
  return NULL;
}

int main(int argc, char **argv) {
  int count;
  uint64_t iterations;
  if (argc != 3 || !lab_parse_int(argv[1], 1, &count) || count > 256 ||
      !lab_u64(argv[2], UINT64_MAX / (unsigned)count, &iterations)) {
    fprintf(stderr, "Usage: %s workers(1..256) iterations\n", argv[0]);
    return 1;
  }
  atomic_uint_fast64_t counter;
  atomic_init(&counter, 0);
  Worker *workers = calloc((size_t)count, sizeof *workers);
  pthread_t *threads = calloc((size_t)count, sizeof *threads);
  if (!workers || !threads) {
    free(workers);
    free(threads);
    return 1;
  }
  int started = 0, result = 0;
  for (; started < count; ++started) {
    workers[started] = (Worker){.counter = &counter, .iterations = iterations};
    if (lab_thread_error(
            pthread_create(&threads[started], NULL, work, &workers[started]),
            "pthread_create")) {
      result = 1;
      break;
    }
  }
  uint64_t joined_total = 0;
  for (int i = 0; i < started; ++i) {
    lab_thread_check(pthread_join(threads[i], NULL), "pthread_join");
    joined_total += workers[i].completed;
  }
  uint64_t total = atomic_load_explicit(&counter, memory_order_relaxed);
  printf("counter=%" PRIu64 " joined_total=%" PRIu64 " lock_free=%d\n", total,
         joined_total, atomic_is_lock_free(&counter));
  if (total != joined_total)
    result = 1;
  free(threads);
  free(workers);
  return result;
}
