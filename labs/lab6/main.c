#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 128
#define MODEL_LEN 50

#define TYPE_NONE 0
#define TYPE_CAR 1
#define TYPE_TRUCK 2

typedef struct {
  char model[MODEL_LEN];
  int type;
} Vehicle;

typedef struct {
  sem_t mutex;
  sem_t car_queue;
  sem_t truck_queue;
  int current_type;
  int last_type;
  int count_inside;
  int spots;
  int waiting_cars;
  int waiting_trucks;
} Garage;

static const char *vehicle_path = "vehicles.txt";

static int parse_positive_int(const char *text, const char *label, int *out);
static int load_vehicles(const char *path, Vehicle *vehicles, int total);
static void simulate_vehicle(const Vehicle *vehicle, Garage *garage);
static void garage_enter(Garage *garage, int type, const char *model);
static void garage_leave(Garage *garage, int type, const char *model);
static void safe_sem_wait(sem_t *sem);
static void safe_sem_post(sem_t *sem);
static void safe_sem_init(sem_t *sem, unsigned int value);
static void wait_for_children(int count);

int main(int argc, char *argv[]) {
  if (argc < 3 || argc > 4) {
    fprintf(
        stderr,
        "Usage: %s <number_of_spots> <number_of_vehicles> [vehicles_file]\n",
        argv[0]);
    return EXIT_FAILURE;
  }

  int spots = 0;
  int total_vehicles = 0;
  if (parse_positive_int(argv[1], "number_of_spots", &spots) != 0 ||
      parse_positive_int(argv[2], "number_of_vehicles", &total_vehicles) != 0) {
    return EXIT_FAILURE;
  }

  if (argc == 4) {
    vehicle_path = argv[3];
  }

  Vehicle *vehicles = calloc((size_t)total_vehicles, sizeof(*vehicles));
  if (!vehicles) {
    perror("calloc vehicles");
    return EXIT_FAILURE;
  }

  if (load_vehicles(vehicle_path, vehicles, total_vehicles) != 0) {
    free(vehicles);
    return EXIT_FAILURE;
  }

  Garage *garage = mmap(NULL, sizeof(*garage), PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (garage == MAP_FAILED) {
    perror("mmap");
    free(vehicles);
    return EXIT_FAILURE;
  }

  safe_sem_init(&garage->mutex, 1);
  safe_sem_init(&garage->car_queue, 0);
  safe_sem_init(&garage->truck_queue, 0);
  garage->current_type = TYPE_NONE;
  garage->last_type = TYPE_TRUCK;
  garage->count_inside = 0;
  garage->spots = spots;
  garage->waiting_cars = 0;
  garage->waiting_trucks = 0;

  for (int i = 0; i < total_vehicles; i++) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      return EXIT_FAILURE;
    }
    if (pid == 0) {
      srand((unsigned int)(time(NULL) ^ (getpid() << 16)));
      usleep((useconds_t)(rand() % 500000));
      simulate_vehicle(&vehicles[i], garage);
      exit(EXIT_SUCCESS);
    }
  }

  wait_for_children(total_vehicles);

  sem_destroy(&garage->mutex);
  sem_destroy(&garage->car_queue);
  sem_destroy(&garage->truck_queue);
  munmap(garage, sizeof(*garage));
  free(vehicles);

  printf("All vehicles have been processed. Garage simulation complete.\n");
  return EXIT_SUCCESS;
}

static int parse_positive_int(const char *text, const char *label, int *out) {
  char *end = NULL;
  errno = 0;
  long value = strtol(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value <= 0 ||
      value > INT_MAX) {
    fprintf(stderr, "Invalid %s: %s\n", label, text);
    return -1;
  }

  *out = (int)value;
  return 0;
}

static int load_vehicles(const char *path, Vehicle *vehicles, int total) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror("fopen vehicles file");
    return -1;
  }

  char line[MAX_LINE_LENGTH];
  int count = 0;
  while (count < total && fgets(line, sizeof(line), fp)) {
    char type_char = '\0';
    char model[MODEL_LEN];
    if (sscanf(line, " %c %49s", &type_char, model) != 2) {
      continue;
    }

    int type = TYPE_NONE;
    if (type_char == 'C' || type_char == 'c') {
      type = TYPE_CAR;
    } else if (type_char == 'T' || type_char == 't') {
      type = TYPE_TRUCK;
    } else {
      continue;
    }

    vehicles[count].type = type;
    strncpy(vehicles[count].model, model, sizeof(vehicles[count].model) - 1);
    vehicles[count].model[sizeof(vehicles[count].model) - 1] = '\0';
    count++;
  }

  fclose(fp);

  if (count < total) {
    fprintf(stderr, "Not enough valid entries in %s. Expected %d, got %d\n",
            path, total, count);
    return -1;
  }

  return 0;
}

static void simulate_vehicle(const Vehicle *vehicle, Garage *garage) {
  char type_char = (vehicle->type == TYPE_CAR) ? 'C' : 'T';
  printf("Vehicle %s (%c) arrives. PID: %d\n", vehicle->model, type_char,
         getpid());

  garage_enter(garage, vehicle->type, vehicle->model);

  sleep(1 + rand() % 3);

  garage_leave(garage, vehicle->type, vehicle->model);
}

static void garage_enter(Garage *garage, int type, const char *model) {
  while (1) {
    safe_sem_wait(&garage->mutex);
    int waiting_same =
        (type == TYPE_CAR) ? garage->waiting_cars : garage->waiting_trucks;
    bool can_enter =
        (garage->current_type == TYPE_NONE || garage->current_type == type) &&
        (garage->count_inside < garage->spots) && (waiting_same == 0);

    if (can_enter) {
      if (garage->current_type == TYPE_NONE) {
        garage->current_type = type;
      }
      garage->count_inside++;
      printf("Vehicle %s (%c) parks. Occupancy: %d/%d\n", model,
             (type == TYPE_CAR) ? 'C' : 'T', garage->count_inside,
             garage->spots);
      safe_sem_post(&garage->mutex);
      return;
    }

    if (type == TYPE_CAR) {
      garage->waiting_cars++;
      safe_sem_post(&garage->mutex);
      safe_sem_wait(&garage->car_queue);
      safe_sem_wait(&garage->mutex);
      garage->waiting_cars--;
    } else {
      garage->waiting_trucks++;
      safe_sem_post(&garage->mutex);
      safe_sem_wait(&garage->truck_queue);
      safe_sem_wait(&garage->mutex);
      garage->waiting_trucks--;
    }

    if (garage->current_type == TYPE_NONE) {
      garage->current_type = type;
    }
    garage->count_inside++;
    printf("Vehicle %s (%c) parks. Occupancy: %d/%d\n", model,
           (type == TYPE_CAR) ? 'C' : 'T', garage->count_inside, garage->spots);
    safe_sem_post(&garage->mutex);
    return;
  }
}

static void garage_leave(Garage *garage, int type, const char *model) {
  safe_sem_wait(&garage->mutex);
  garage->count_inside--;
  printf("Vehicle %s (%c) leaves. Remaining: %d\n", model,
         (type == TYPE_CAR) ? 'C' : 'T', garage->count_inside);

  if (garage->count_inside == 0) {
    garage->current_type = TYPE_NONE;

    int cars_waiting = garage->waiting_cars;
    int trucks_waiting = garage->waiting_trucks;

    if (cars_waiting > 0 || trucks_waiting > 0) {
      int next_type = TYPE_NONE;
      if (cars_waiting > 0 && trucks_waiting > 0) {
        next_type = (garage->last_type == TYPE_CAR) ? TYPE_TRUCK : TYPE_CAR;
      } else if (cars_waiting > 0) {
        next_type = TYPE_CAR;
      } else {
        next_type = TYPE_TRUCK;
      }

      garage->current_type = next_type;
      garage->last_type = next_type;

      int slots = garage->spots;
      if (next_type == TYPE_CAR) {
        if (slots > cars_waiting) {
          slots = cars_waiting;
        }
        for (int i = 0; i < slots; i++) {
          safe_sem_post(&garage->car_queue);
        }
      } else {
        if (slots > trucks_waiting) {
          slots = trucks_waiting;
        }
        for (int i = 0; i < slots; i++) {
          safe_sem_post(&garage->truck_queue);
        }
      }
    }
  } else {
    if (garage->current_type == TYPE_CAR && garage->waiting_cars > 0 &&
        garage->count_inside < garage->spots) {
      safe_sem_post(&garage->car_queue);
    } else if (garage->current_type == TYPE_TRUCK &&
               garage->waiting_trucks > 0 &&
               garage->count_inside < garage->spots) {
      safe_sem_post(&garage->truck_queue);
    }
  }

  safe_sem_post(&garage->mutex);
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

static void safe_sem_init(sem_t *sem, unsigned int value) {
  if (sem_init(sem, 1, value) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }
}

static void wait_for_children(int count) {
  for (int i = 0; i < count; i++) {
    while (waitpid(-1, NULL, 0) == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("waitpid");
      return;
    }
  }
}
