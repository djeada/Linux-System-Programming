/* C11/POSIX: FIFO waiting chairs and a completion semaphore per customer.
   Children cannot report a finished haircut until their own service completes.
 */
#include "../common/process.h"

typedef struct {
  sem_t finished;
  int next;
} Customer;
typedef struct {
  sem_t mutex, arrivals;
  int chairs, waiting, head, tail, arrivals_left;
  Customer customers[];
} Shop;

static void barber(Shop *shop) {
  for (;;) {
    lab_sem_wait(&shop->arrivals);
    lab_sem_wait(&shop->mutex);
    if (shop->head < 0) {
      bool done = shop->arrivals_left == 0;
      lab_sem_post(&shop->mutex);
      if (done)
        break;
      continue;
    }
    int customer = shop->head;
    shop->head = shop->customers[customer].next;
    if (shop->head < 0)
      shop->tail = -1;
    --shop->waiting;
    lab_sem_post(&shop->mutex);
    printf("Barber: Starting haircut for customer %d.\n", customer + 1);
    lab_simulate_ms(1000);
    printf("Barber: Finished haircut for customer %d.\n", customer + 1);
    fflush(stdout);
    lab_sem_post(&shop->customers[customer].finished);
  }
  puts("Barber: Closing shop.");
}

static void customer(Shop *shop, int id) {
  lab_sem_wait(&shop->mutex);
  --shop->arrivals_left;
  bool accepted = shop->waiting < shop->chairs;
  if (accepted) {
    if (shop->tail < 0)
      shop->head = id;
    else
      shop->customers[shop->tail].next = id;
    shop->tail = id;
    ++shop->waiting;
    printf("Customer %d: Sitting in waiting area.\n", id + 1);
    lab_sem_post(&shop->arrivals);
  }
  /* Wake an idle barber after the last admission, even if everyone left. */
  if (shop->arrivals_left == 0)
    lab_sem_post(&shop->arrivals);
  lab_sem_post(&shop->mutex);
  if (!accepted) {
    printf("Customer %d: No available chairs. Leaving the shop.\n", id + 1);
    return;
  }
  lab_sem_wait(&shop->customers[id].finished);
  printf("Customer %d: Haircut done, paying at the cashier.\n", id + 1);
}

int main(int argc, char *argv[]) {
  int chairs, customers;
  if (argc != 3 || !lab_parse_int(argv[1], 0, &chairs) ||
      !lab_parse_int(argv[2], 0, &customers)) {
    fprintf(stderr, "Usage: %s <nonnegative_chairs> <nonnegative_customers>\n",
            argv[0]);
    return EXIT_FAILURE;
  }
  if ((size_t)customers > (SIZE_MAX - sizeof(Shop)) / sizeof(Customer))
    return EXIT_FAILURE;
  size_t bytes = sizeof(Shop) + (size_t)customers * sizeof(Customer);
  pid_t *children = calloc((size_t)customers + 1, sizeof(*children));
  if (!children) {
    perror("calloc children");
    return EXIT_FAILURE;
  }
  Shop *shop = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shop == MAP_FAILED) {
    perror("mmap");
    free(children);
    return EXIT_FAILURE;
  }
  *shop = (Shop){
      .chairs = chairs, .head = -1, .tail = -1, .arrivals_left = customers};
  sem_t *base[] = {&shop->mutex, &shop->arrivals};
  unsigned int values[] = {1, customers == 0 ? 1U : 0U};
  if (!lab_sem_initialize(base, values, 2)) {
    munmap(shop, bytes);
    free(children);
    return EXIT_FAILURE;
  }
  int initialized = 0;
  for (; initialized < customers; ++initialized) {
    shop->customers[initialized].next = -1;
    if (sem_init(&shop->customers[initialized].finished, 1, 0) < 0) {
      perror("sem_init customer");
      break;
    }
  }
  int result = EXIT_FAILURE;
  size_t started = 0;
  if (initialized != customers)
    goto cleanup;
  for (int id = -1; id < customers; ++id) {
    pid_t pid = lab_fork();
    if (pid < 0) {
      perror("fork");
      lab_terminate_children(children, started);
      goto cleanup;
    }
    if (pid == 0) {
      if (id < 0)
        barber(shop);
      else
        customer(shop, id);
      lab_child_exit(EXIT_SUCCESS);
    }
    children[started++] = pid;
  }
  if (lab_supervise_children(children, started)) {
    puts("Simulation complete.");
    result = EXIT_SUCCESS;
  }
cleanup:
  for (int id = 0; id < initialized; ++id)
    sem_destroy(&shop->customers[id].finished);
  sem_destroy(&shop->arrivals);
  sem_destroy(&shop->mutex);
  munmap(shop, bytes);
  free(children);
  return result;
}
