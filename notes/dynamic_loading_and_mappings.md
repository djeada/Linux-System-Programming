# Dynamic loading and file-backed memory

Companion labs: [18: plugin ABI](../labs/lab18/README.md) and
[19: mappings](../labs/lab19/README.md). Both require an explicit lifetime for
addresses that refer to an owned external resource.

## A shared object is executable code in the host

Build position-independent plugin code with `-fPIC -shared`. Load a trusted
explicit path with `RTLD_NOW | RTLD_LOCAL`: resolve needed symbols now and keep
this object's symbols out of the global lookup scope. These flags do not
isolate memory, permissions, or constructors. `dlopen` can execute constructors
before the host validates a descriptor.

The versioned C descriptor defines the ABI shared by host and plugin. Exporting
a data descriptor also avoids converting an ISO C object pointer directly to
a function pointer. Clear `dlerror`, perform `dlsym`, then examine `dlerror`;
a null symbol value alone is not the complete error protocol. Validate the
version and expected fields before use. Finish callbacks and release all
plugin-owned objects before `dlclose`; pointers into the library cannot be
assumed usable afterward. See
[dlopen(3)](https://man7.org/linux/man-pages/man3/dlopen.3.html) and
[dlsym(3)](https://man7.org/linux/man-pages/man3/dlsym.3.html).

If the API grows, specify structure sizes/versioning, allocator ownership,
thread safety, error representation, and unload behavior. A C struct does not
automatically make different architecture/compiler ABIs compatible.

## A mapping has bounds, protection, and persistence

`mmap` maps a nonempty range; its file offset must meet page alignment rules.
Lab19 maps from offset zero and checks the requested edit against the file
size. `MAP_PRIVATE` uses copy-on-write for edits and does not save those edits
to the backing file. `MAP_SHARED` edits can affect the backing object and other
observers. Neither flag is a mutex or a transaction.

The shared mode synchronizes mapped changes with `msync(MS_SYNC)` and then
syncs the file. `mprotect` changes this mapping's access permissions; it does
not establish thread synchronization or protect the file from another process.
`munmap` releases the address range, after which pointers into it are invalid.
See [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html),
[msync(2)](https://man7.org/linux/man-pages/man2/msync.2.html), and
[mprotect(2)](https://man7.org/linux/man-pages/man2/mprotect.2.html).

Lab19 takes an advisory file lock for cooperating users. A process that ignores
the protocol can still truncate the file, making a later mapped access raise
SIGBUS. Bounds checks at startup do not eliminate that race. Shared edits can
become visible in pieces; whole-file atomic replacement belongs to lab10's
different protocol. Memory mapping is a choice to measure against ordinary
I/O, not an automatic optimization for every file workload.
