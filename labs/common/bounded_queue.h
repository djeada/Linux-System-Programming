#ifndef LAB_BOUNDED_QUEUE_H
#define LAB_BOUNDED_QUEUE_H

#include "system.h"

typedef struct {
  unsigned char *data;
  size_t capacity, item_size, head, count;
  bool closed;
  pthread_mutex_t mutex;
  pthread_cond_t readable, writable;
} LabQueue;

static inline bool lab_queue_init(LabQueue *queue, size_t capacity,
                                  size_t item_size) {
  if (!capacity || !item_size || capacity > SIZE_MAX / item_size)
    return false;
  *queue = (LabQueue){.capacity = capacity, .item_size = item_size};
  queue->data = calloc(capacity, item_size);
  if (!queue->data) {
    perror("calloc queue");
    return false;
  }
  int code = pthread_mutex_init(&queue->mutex, NULL);
  if (code)
    goto fail_data;
  pthread_condattr_t attr;
  code = pthread_condattr_init(&attr);
  if (code)
    goto fail_mutex;
  code = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
  if (code)
    goto fail_attr;
  code = pthread_cond_init(&queue->readable, &attr);
  if (code)
    goto fail_attr;
  code = pthread_cond_init(&queue->writable, &attr);
  if (code) {
    pthread_cond_destroy(&queue->readable);
    goto fail_attr;
  }
  pthread_condattr_destroy(&attr);
  return true;
fail_attr:
  pthread_condattr_destroy(&attr);
fail_mutex:
  pthread_mutex_destroy(&queue->mutex);
fail_data:
  lab_thread_error(code, "initialize queue");
  free(queue->data);
  queue->data = NULL;
  return false;
}

static inline void lab_queue_close(LabQueue *queue) {
  lab_thread_check(pthread_mutex_lock(&queue->mutex), "lock queue");
  queue->closed = true;
  lab_thread_check(pthread_cond_broadcast(&queue->readable), "wake readers");
  lab_thread_check(pthread_cond_broadcast(&queue->writable), "wake writers");
  lab_thread_check(pthread_mutex_unlock(&queue->mutex), "unlock queue");
}

static inline bool lab_queue_push(LabQueue *queue, const void *item,
                                  const struct timespec *deadline) {
  lab_thread_check(pthread_mutex_lock(&queue->mutex), "lock queue");
  while (queue->count == queue->capacity && !queue->closed) {
    int code = deadline ? pthread_cond_timedwait(&queue->writable,
                                                 &queue->mutex, deadline)
                        : pthread_cond_wait(&queue->writable, &queue->mutex);
    if (code == ETIMEDOUT) {
      pthread_mutex_unlock(&queue->mutex);
      return false;
    }
    lab_thread_check(code, "wait for queue capacity");
  }
  bool accepted =
      !queue->closed && (!deadline || lab_before(lab_now(), *deadline));
  if (accepted) {
    const size_t tail = (queue->head + queue->count) % queue->capacity;
    memcpy(queue->data + tail * queue->item_size, item, queue->item_size);
    ++queue->count;
    lab_thread_check(pthread_cond_signal(&queue->readable), "wake reader");
  }
  lab_thread_check(pthread_mutex_unlock(&queue->mutex), "unlock queue");
  return accepted;
}

static inline bool lab_queue_pop(LabQueue *queue, void *item) {
  lab_thread_check(pthread_mutex_lock(&queue->mutex), "lock queue");
  while (!queue->count && !queue->closed)
    lab_thread_check(pthread_cond_wait(&queue->readable, &queue->mutex),
                     "wait for work");
  bool available = queue->count != 0;
  if (available) {
    memcpy(item, queue->data + queue->head * queue->item_size,
           queue->item_size);
    queue->head = (queue->head + 1) % queue->capacity;
    --queue->count;
    lab_thread_check(pthread_cond_signal(&queue->writable), "wake writer");
  }
  lab_thread_check(pthread_mutex_unlock(&queue->mutex), "unlock queue");
  return available;
}

static inline size_t lab_queue_count(LabQueue *queue) {
  lab_thread_check(pthread_mutex_lock(&queue->mutex), "lock queue");
  size_t count = queue->count;
  lab_thread_check(pthread_mutex_unlock(&queue->mutex), "unlock queue");
  return count;
}

/* Call only after every producer and consumer has joined. */
static inline void lab_queue_destroy(LabQueue *queue) {
  lab_thread_check(pthread_cond_destroy(&queue->writable), "destroy writable");
  lab_thread_check(pthread_cond_destroy(&queue->readable), "destroy readable");
  lab_thread_check(pthread_mutex_destroy(&queue->mutex), "destroy queue mutex");
  free(queue->data);
}

#endif
