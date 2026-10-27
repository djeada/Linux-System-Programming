/* Memory-model exercise. Prefer a condition variable for application waiting.
 */
#include "../../common/system.h"
#include <stdatomic.h>

typedef struct {
  atomic_bool ready;
  int payload;
} Message;
static void *publish(void *argument) {
  Message *message = argument;
  message->payload = 42;
  atomic_store_explicit(&message->ready, true, memory_order_release);
  return NULL;
}
int main(void) {
  Message message;
  atomic_init(&message.ready, false);
  pthread_t producer;
  if (lab_thread_error(pthread_create(&producer, NULL, publish, &message),
                       "pthread_create"))
    return 1;
  while (!atomic_load_explicit(&message.ready, memory_order_acquire))
    lab_sleep_ms(1);
  printf("published=%d\n", message.payload);
  lab_thread_check(pthread_join(producer, NULL), "pthread_join");
  return message.payload != 42;
}
