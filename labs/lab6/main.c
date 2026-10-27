/* C11/POSIX: phase-fair admissions with capacity reserved before waking.
   Each vehicle owns one gate. Type batches alternate when both types wait;
   arrivals cannot consume permits reserved for already selected vehicles. */
#include "../common/process.h"

#define MODEL_LEN 50
#define TYPE_CAR 1
#define TYPE_TRUCK 2

typedef struct {
  char model[MODEL_LEN];
  int type, next;
  sem_t admitted;
} Vehicle;
typedef struct {
  sem_t mutex;
  int spots, inside, current_type, last_type;
  int head[3], tail[3], waiting[3];
  Vehicle vehicles[];
} Garage;

/* Caller holds the state mutex. inside includes issued reservations. */
static void admit_waiters(Garage *garage) {
  bool new_phase = garage->inside == 0;
  if (new_phase) {
    int selected = 0;
    if (garage->waiting[TYPE_CAR] && garage->waiting[TYPE_TRUCK])
      selected = garage->last_type == TYPE_CAR ? TYPE_TRUCK : TYPE_CAR;
    else if (garage->waiting[TYPE_CAR])
      selected = TYPE_CAR;
    else if (garage->waiting[TYPE_TRUCK])
      selected = TYPE_TRUCK;
    garage->current_type = selected;
    if (!selected)
      return;
    garage->last_type = selected;
  }
  int type = garage->current_type;
  int other = type == TYPE_CAR ? TYPE_TRUCK : TYPE_CAR;
  /* Stop extending an active batch once the opposite type is waiting. */
  if (!new_phase && garage->waiting[other])
    return;
  while (garage->inside < garage->spots && garage->head[type] >= 0) {
    int id = garage->head[type];
    garage->head[type] = garage->vehicles[id].next;
    if (garage->head[type] < 0)
      garage->tail[type] = -1;
    --garage->waiting[type];
    ++garage->inside; /* Reserve now, before the selected process wakes. */
    lab_sem_post(&garage->vehicles[id].admitted);
  }
}

static void simulate_vehicle(Garage *garage, int id) {
  Vehicle *vehicle = &garage->vehicles[id];
  int type = vehicle->type;
  lab_sem_wait(&garage->mutex);
  if (garage->tail[type] < 0)
    garage->head[type] = id;
  else
    garage->vehicles[garage->tail[type]].next = id;
  garage->tail[type] = id;
  ++garage->waiting[type];
  admit_waiters(garage);
  lab_sem_post(&garage->mutex);

  lab_sem_wait(&vehicle->admitted);
  lab_sem_wait(&garage->mutex);
  printf("Vehicle %s (%c) parks. Occupancy: %d/%d\n", vehicle->model,
         type == TYPE_CAR ? 'C' : 'T', garage->inside, garage->spots);
  fflush(stdout);
  lab_sem_post(&garage->mutex);
  lab_simulate_ms(1000);
  lab_sem_wait(&garage->mutex);
  --garage->inside;
  printf("Vehicle %s (%c) leaves. Remaining: %d\n", vehicle->model,
         type == TYPE_CAR ? 'C' : 'T', garage->inside);
  fflush(stdout);
  admit_waiters(garage);
  lab_sem_post(&garage->mutex);
}

static bool load_vehicles(FILE *file, Garage *garage, int total) {
  char *line = NULL;
  size_t capacity = 0;
  int count = 0;
  while (count < total && getline(&line, &capacity, file) >= 0) {
    char type, extra, model[MODEL_LEN];
    if (sscanf(line, " %c %49s %c", &type, model, &extra) != 2)
      continue;
    if (type != 'C' && type != 'c' && type != 'T' && type != 't')
      continue;
    Vehicle *vehicle = &garage->vehicles[count++];
    vehicle->type = type == 'C' || type == 'c' ? TYPE_CAR : TYPE_TRUCK;
    memcpy(vehicle->model, model, strlen(model) + 1);
    vehicle->next = -1;
  }
  free(line);
  if (ferror(file)) {
    perror("read vehicles");
    return false;
  }
  if (count != total) {
    fprintf(stderr, "Not enough valid vehicles: expected %d, got %d\n", total,
            count);
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  int spots, total;
  if ((argc != 3 && argc != 4) || !lab_parse_int(argv[1], 1, &spots) ||
      !lab_parse_int(argv[2], 1, &total)) {
    fprintf(stderr,
            "Usage: %s <positive_spots> <positive_vehicles> [vehicles_file]\n",
            argv[0]);
    return EXIT_FAILURE;
  }
  const char *path = argc == 4 ? argv[3] : "vehicles.txt";
  FILE *file = fopen(path, "r");
  if (!file) {
    perror("open vehicles");
    return EXIT_FAILURE;
  }
  if ((size_t)total > (SIZE_MAX - sizeof(Garage)) / sizeof(Vehicle)) {
    fclose(file);
    return EXIT_FAILURE;
  }
  size_t bytes = sizeof(Garage) + (size_t)total * sizeof(Vehicle);
  Garage *garage = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (garage == MAP_FAILED) {
    perror("mmap");
    fclose(file);
    return EXIT_FAILURE;
  }
  *garage = (Garage){.spots = spots,
                     .last_type = TYPE_TRUCK,
                     .head = {-1, -1, -1},
                     .tail = {-1, -1, -1}};
  if (!load_vehicles(file, garage, total)) {
    fclose(file);
    munmap(garage, bytes);
    return EXIT_FAILURE;
  }
  fclose(file);
  pid_t *children = calloc((size_t)total, sizeof(*children));
  if (!children) {
    perror("calloc children");
    munmap(garage, bytes);
    return EXIT_FAILURE;
  }
  if (sem_init(&garage->mutex, 1, 1) < 0) {
    perror("sem_init");
    free(children);
    munmap(garage, bytes);
    return EXIT_FAILURE;
  }
  int initialized = 0;
  for (; initialized < total; ++initialized)
    if (sem_init(&garage->vehicles[initialized].admitted, 1, 0) < 0) {
      perror("sem_init gate");
      break;
    }
  int result = EXIT_FAILURE;
  size_t started = 0;
  if (initialized != total)
    goto cleanup;
  for (int id = 0; id < total; ++id) {
    pid_t pid = lab_fork();
    if (pid < 0) {
      perror("fork");
      lab_terminate_children(children, started);
      goto cleanup;
    }
    if (pid == 0) {
      simulate_vehicle(garage, id);
      lab_child_exit(EXIT_SUCCESS);
    }
    children[started++] = pid;
  }
  if (lab_supervise_children(children, started)) {
    puts("All vehicles have been processed. Garage simulation complete.");
    result = EXIT_SUCCESS;
  }
cleanup:
  for (int id = 0; id < initialized; ++id)
    sem_destroy(&garage->vehicles[id].admitted);
  sem_destroy(&garage->mutex);
  free(children);
  munmap(garage, bytes);
  return result;
}
