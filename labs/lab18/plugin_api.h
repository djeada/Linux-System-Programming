#ifndef LAB_PLUGIN_API_H
#define LAB_PLUGIN_API_H
#define LAB_PLUGIN_ABI 1u
typedef struct {
  unsigned abi;
  const char *name;
  int (*compute)(int left, int right, int *result);
} LabPlugin;
extern const LabPlugin lab_plugin;
#endif
