# Lab 19: File-backed mappings

Compare copy-on-write and shared file mappings, protection changes, and
explicit persistence. See [mapping notes](../../notes/dynamic_loading_and_mappings.md).

```sh
make build/lab19
printf 'hello world' > /tmp/lab19-file
./build/lab19 /tmp/lab19-file private 6 earth
cat /tmp/lab19-file
./build/lab19 /tmp/lab19-file shared 6 earth
cat /tmp/lab19-file
```

The private run leaves `hello world` on disk; the shared run produces
`hello earth`. Arguments are `file shared|private offset text`. The file must
already be a nonempty regular file, and the replacement must fit entirely
inside it. This lab does not resize files or append. Empty text is valid.

Main obtains a whole-file advisory lock, checks metadata/bounds, maps from
page-aligned offset zero, modifies the requested bytes, and for shared mode
calls `msync(MS_SYNC)` then `fsync`. It changes the mapping to read-only with
`mprotect`, verifies the bytes, unmaps, and closes the descriptor/lock.

**Current pattern:** validate the mapped range, define concurrent access, and
separate visibility, protection, and durability. **Fragile recipe:** treat
`MAP_PRIVATE` edits as saved data, assume a mapping prevents truncation, or
use `msync` as thread synchronization.

The lock coordinates only cooperating users. External truncation can cause
`SIGBUS` even after a successful bounds check. Avoid concurrent uncooperative
writers for this exercise. Shared writes modify the live file and can be
partially visible or persistent on failure; use lab10's replacement protocol
when a whole-file update must become visible together. Explore `/proc/PID/maps`
in GDB before/after `mprotect` without intentionally crashing the default lab.
