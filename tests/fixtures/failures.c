#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* Linux-only fault injection into lab executables, never the test runner. */
static int selected(const char *setting, int call) {
  const char *value = getenv(setting);
  return value && strtol(value, NULL, 10) == call;
}

pid_t fork(void) {
  static int calls;
  ++calls;
  if (selected("LAB_FAIL_FORK", calls)) {
    errno = EAGAIN;
    return -1;
  }
  pid_t (*original)(void) = (pid_t(*)(void))dlsym(RTLD_NEXT, "fork");
  pid_t child = original();
  if (child == 0 && selected("LAB_FAIL_CHILD", calls))
    _exit(91);
  return child;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*work)(void *), void *context) {
  static int calls;
  if (selected("LAB_FAIL_THREAD", ++calls)) {
    errno = 0;
    return EAGAIN;
  }
  int (*original)(pthread_t *, const pthread_attr_t *, void *(*)(void *),
                  void *) =
      (int (*)(pthread_t *, const pthread_attr_t *, void *(*)(void *),
               void *))dlsym(RTLD_NEXT, "pthread_create");
  return original(thread, attr, work, context);
}

int sem_init(sem_t *sem, int shared, unsigned int value) {
  static int calls;
  if (selected("LAB_FAIL_SEMAPHORE", ++calls)) {
    errno = ENOSPC;
    return -1;
  }
  int (*original)(sem_t *, int, unsigned int) =
      (int (*)(sem_t *, int, unsigned int))dlsym(RTLD_NEXT, "sem_init");
  return original(sem, shared, value);
}

int pthread_cond_init(pthread_cond_t *condition,
                      const pthread_condattr_t *attr) {
  static int calls;
  if (selected("LAB_FAIL_CONDITION", ++calls))
    return EAGAIN;
  int (*original)(pthread_cond_t *, const pthread_condattr_t *) =
      (int (*)(pthread_cond_t *, const pthread_condattr_t *))dlsym(
          RTLD_NEXT, "pthread_cond_init");
  return original(condition, attr);
}
