# Lab 18: Shared libraries and runtime loading

Define a small versioned C plugin ABI and own its lifetime explicitly. Read
[loading and mapping notes](../../notes/dynamic_loading_and_mappings.md).

```sh
make build/lab18 build/lab18_plugin.so
./build/lab18 ./build/lab18_plugin.so 19 23
```

The shared library exports a `const LabPlugin lab_plugin` data descriptor with
an ABI number, name, and checked-add function pointer. The loader uses
`dlopen(RTLD_NOW | RTLD_LOCAL)`, clears/checks `dlerror` around `dlsym`, validates
the ABI, calls the function, and closes the library afterward. An explicit
path containing `/` avoids searching for an unrelated library by name.
Overflow is reported as `ERANGE`; malformed numbers, missing libraries, and
incompatible descriptors fail with a diagnostic.

To compile separately:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -fPIC -shared labs/lab18/plugin.c -o build/lab18_plugin.so
cc -std=c11 -Wall -Wextra -Wpedantic labs/lab18/main.c -pthread -ldl -o build/lab18
```

**Current pattern:** a documented descriptor and explicit ABI/lifetime contract.
**Fragile recipe:** keep a function or string pointer after unloading its
library, assume a null symbol is the only error check, or mix compiler-specific
C++ ABIs accidentally.

Only load trusted modules. Loading runs code in the host process, including
constructors before ABI validation; this is not a sandbox or validation of
arbitrary ELF files. Exercises: add an ABI-compatible operation; build a second
descriptor with the wrong version and confirm rejection. Keep allocator and
ownership rules explicit if plugins later return allocated objects.
