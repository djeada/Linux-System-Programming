#include "plugin_api.h"
#include <errno.h>
#include <limits.h>

static int add(int left, int right, int *result) {
  if ((right > 0 && left > INT_MAX - right) ||
      (right < 0 && left < INT_MIN - right))
    return ERANGE;
  *result = left + right;
  return 0;
}
const LabPlugin lab_plugin = {
    .abi = LAB_PLUGIN_ABI, .name = "checked-add", .compute = add};
