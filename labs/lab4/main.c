#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  int waiting_customers;
  int chairs;
  int shutdown;
  sem_t mutex;
  sem_t customer_sem;
  sem_t barber_sem;
} SharedData;

static void barber(SharedData *shared);
static void customer(SharedData *shared, int customer_id);
static int parse_nonneg_int(const char *text, const char *label,
                            int *value_out);
static void safe_sem_wait(sem_t *sem);
static void safe_sem_post(sem_t *sem);

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <number_of_chairs> <number_of_customers>\n",
            argv[0]);
    return EXIT_FAILURE;
  }

  int chairs = 0;
  int total_customers = 0;
  if (parse_nonneg_int(argv[1], "number_of_chairs", &chairs) != 0 ||
      parse_nonneg_int(argv[2], "number_of_customers", &total_customers) != 0) {
    return EXIT_FAILURE;
  }

  SharedData *shared = mmap(NULL, sizeof(*shared), PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shared == MAP_FAILED) {
    perror("mmap");
    return EXIT_FAILURE;
  }

  shared->waiting_customers = 0;
  shared->chairs = chairs;
  shared->shutdown = 0;

  if (sem_init(&shared->mutex, 1, 1) == -1 ||
      sem_init(&shared->customer_sem, 1, 0) == -1 ||
      sem_init(&shared->barber_sem, 1, 0) == -1) {
    perror("sem_init");
    munmap(shared, sizeof(*shared));
    return EXIT_FAILURE;
  }

  pid_t barber_pid = fork();
  if (barber_pid < 0) {
    perror("fork");
    munmap(shared, sizeof(*shared));
    return EXIT_FAILURE;
  }

  if (barber_pid == 0) {
    barber(shared);
    exit(EXIT_SUCCESS);
  }

  pid_t *customer_pids = NULL;
  if (total_customers > 0) {
    customer_pids = calloc((size_t)total_customers, sizeof(*customer_pids));
    if (!customer_pids) {
      perror("calloc");
      return EXIT_FAILURE;
    }
  }

  for (int i = 0; i < total_customers; i++) {
    pid_t customer_pid = fork();
    if (customer_pid < 0) {
      perror("fork");
      return EXIT_FAILURE;
    }
    if (customer_pid == 0) {
      srand((unsigned int)(time(NULL) ^ (getpid() << 16)));
      usleep((useconds_t)(rand() % 500000));
      customer(shared, i + 1);
      exit(EXIT_SUCCESS);
    }
    customer_pids[i] = customer_pid;
  }

  for (int i = 0; i < total_customers; i++) {
    int status = 0;
    while (waitpid(customer_pids[i], &status, 0) == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("waitpid");
      break;
    }
  }

  free(customer_pids);

  safe_sem_wait(&shared->mutex);
  shared->shutdown = 1;
  safe_sem_post(&shared->mutex);
  safe_sem_post(&shared->customer_sem);

  while (waitpid(barber_pid, NULL, 0) == -1) {
    if (errno == EINTR) {
      continue;
    }
    perror("waitpid");
    break;
  }

  sem_destroy(&shared->mutex);
  sem_destroy(&shared->customer_sem);
  sem_destroy(&shared->barber_sem);
  munmap(shared, sizeof(*shared));

  printf("Simulation complete.\n");
  return EXIT_SUCCESS;
}

static void barber(SharedData *shared) {
  printf("Barber: Starting work. Waiting for customers...\n");
  for (;;) {
    safe_sem_wait(&shared->customer_sem);

    safe_sem_wait(&shared->mutex);
    if (shared->shutdown && shared->waiting_customers == 0) {
      safe_sem_post(&shared->mutex);
      break;
    }

    shared->waiting_customers--;
    printf("Barber: Starting haircut. Waiting customers: %d\n",
           shared->waiting_customers);
    safe_sem_post(&shared->barber_sem);
    safe_sem_post(&shared->mutex);

    sleep(1);
    printf("Barber: Finished haircut.\n");
  }

  printf("Barber: Closing shop.\n");
}

static void customer(SharedData *shared, int customer_id) {
  printf("Customer %d: Arrived at the barbershop.\n", customer_id);

  safe_sem_wait(&shared->mutex);
  if (shared->waiting_customers < shared->chairs) {
    shared->waiting_customers++;
    printf("Customer %d: Sitting in waiting area. Waiting customers: %d\n",
           customer_id, shared->waiting_customers);
    safe_sem_post(&shared->customer_sem);
    safe_sem_post(&shared->mutex);

    safe_sem_wait(&shared->barber_sem);
    printf("Customer %d: Getting a haircut.\n", customer_id);
    printf("Customer %d: Haircut done, paying at the cashier.\n", customer_id);
    return;
  }

  safe_sem_post(&shared->mutex);
  printf("Customer %d: No available chairs. Leaving the shop.\n", customer_id);
}

static int parse_nonneg_int(const char *text, const char *label,
                            int *value_out) {
  char *end = NULL;
  errno = 0;
  long value = strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value < 0 ||
      value > INT_MAX) {
    fprintf(stderr, "Invalid %s: %s\n", label, text);
    return -1;
  }

  *value_out = (int)value;
  return 0;
}

static void safe_sem_wait(sem_t *sem) {
  while (sem_wait(sem) == -1) {
    if (errno == EINTR) {
      continue;
    }
    perror("sem_wait");
    exit(EXIT_FAILURE);
  }
}

static void safe_sem_post(sem_t *sem) {
  if (sem_post(sem) == -1) {
    perror("sem_post");
    exit(EXIT_FAILURE);
  }
}
