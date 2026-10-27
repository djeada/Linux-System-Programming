/* Deliberately incorrect: use only for the LeakSanitizer exercise. */
#include <stdio.h>
#include <stdlib.h>
int main(void) {
  void *allocation = malloc(128);
  if (!allocation)
    return 1;
  printf("Leaked allocation: %p\n", allocation);
  return 0; /* Missing free(allocation). */
}
