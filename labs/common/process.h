#ifndef LAB_PROCESS_H
#define LAB_PROCESS_H

#include "system.h"
#include <semaphore.h>
#include <sys/mman.h>

static inline bool lab_sem_initialize(sem_t **semaphores,
                                      const unsigned int *values,
                                      size_t count) {
  size_t initialized = 0;
  for (; initialized < count; ++initialized) {
    if (sem_init(semaphores[initialized], 1, values[initialized]) == 0)
      continue;
    int error = errno;
    while (initialized)
      sem_destroy(semaphores[--initialized]);
    errno = error;
    perror("sem_init");
    return false;
  }
  return true;
}

static inline void lab_sem_wait(sem_t *semaphore) {
  while (sem_wait(semaphore) < 0) {
    if (errno == EINTR)
      continue;
    perror("sem_wait");
    lab_child_exit(EXIT_FAILURE);
  }
}

static inline void lab_sem_post(sem_t *semaphore) {
  if (sem_post(semaphore) < 0) {
    perror("sem_post");
    lab_child_exit(EXIT_FAILURE);
  }
}

#endif
