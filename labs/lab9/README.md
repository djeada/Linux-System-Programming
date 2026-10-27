# Lab 9: File descriptors and complete I/O

Implement a binary file copier using `open`, `read`, `write`, and `close`.
Then copy a range using `pread`/`pwrite` and observe the descriptor offsets.
Read [the I/O notes](../../notes/file_io_and_filesystems.md) first.

From the repository root:

```sh
make build/lab9
printf '0123456789\n' > /tmp/lab9-source
./build/lab9 /tmp/lab9-source /tmp/lab9-copy
./build/lab9 /tmp/lab9-source /tmp/lab9-range 2 4
cat /tmp/lab9-range
```

The range result is `2345`; both current offsets remain zero. Reaching EOF
before the requested range ends is a successful shorter copy. Empty files and
zero-length ranges work. Paths must identify different regular files: hard
link aliases are rejected before truncating the destination. A destination
symlink is rejected. Newly created destinations use mode `0600` subject to umask.

**Current pattern:** advance by the actual byte count, retry `EINTR`, keep each
descriptor's owner clear, and set `O_CLOEXEC` when opening. `dup`/`fork` can
share an open file description and its offset; two independent opens do not.

**Fragile recipe:** assume one `write` consumes the entire buffer, use `strlen`
for binary data, or truncate before checking source/destination identity.

This copier can leave a partial destination on failure and does not promise
crash durability. Lab10 introduces replacement rather than overwriting. The
tests inject short reads/writes and interruptions into both I/O paths. Extend
the exercise by comparing offsets after `dup`, and inspect `/proc/self/fd`
under a debugger.
