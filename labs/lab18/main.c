#include "../common/io.h"
#include "plugin_api.h"
#include <dlfcn.h>

int main(int argc, char **argv) {
  int left, right;
  if (argc != 4 || !lab_parse_int(argv[2], INT_MIN, &left) ||
      !lab_parse_int(argv[3], INT_MIN, &right)) {
    fprintf(stderr, "Usage: %s trusted_plugin.so left right\n", argv[0]);
    return 1;
  }
  void *library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!library) {
    fprintf(stderr, "dlopen: %s\n", dlerror());
    return 1;
  }
  dlerror();
  /* Export a data descriptor: no nonportable function-pointer cast is needed.
   */
  const LabPlugin *plugin = dlsym(library, "lab_plugin");
  const char *error = dlerror();
  int result = 1;
  if (error)
    fprintf(stderr, "dlsym: %s\n", error);
  else if (!plugin || plugin->abi != LAB_PLUGIN_ABI || !plugin->name ||
           !plugin->compute)
    fprintf(stderr, "Incompatible plugin descriptor\n");
  else {
    int value, code = plugin->compute(left, right, &value);
    if (code)
      fprintf(stderr, "plugin: %s\n", strerror(code));
    else {
      printf("plugin=%s result=%d\n", plugin->name, value);
      result = 0;
    }
  }
  /* Every pointer into the plugin becomes invalid when dlclose unloads it. */
  if (dlclose(library) != 0) {
    fprintf(stderr, "dlclose: %s\n", dlerror());
    result = 1;
  }
  return result;
}
