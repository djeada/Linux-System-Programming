# Forked File Reader

## Objective

Practice process creation with `fork()` by having a parent and child read the same file independently and print each line with their PID.

## What You Must Implement

1. **Process creation**
   - Call `fork()` once to create a child process.
   - Both parent and child should execute the same reading logic.

2. **File reading**
   - Accept the input file path on the command line. Open it independently in each process after `fork()`.
   - Read the file line by line using standard I/O.
   - Print each line prefixed with the process ID, e.g.:
     ```
     [PID 12345] This is a line from the file.
     ```

3. **Error handling**
   - Check `fork`, file reads, and child status. The parent reaps its child even if its own file open fails.

## Hints

- Each process should open the file independently after `fork()`.
- Output from the two processes may interleave; that is expected.
- Wait for the child with `waitpid` and retry `EINTR`. No shared-file locking or IPC is needed for independent read-only streams.
- Use `getline` so a long logical line gets one PID prefix, rather than being silently split into fixed-size chunks.

## Build and Run

```sh
cc -std=c11 -Wall -Wextra -pedantic -o forked_file_reader main.c -pthread
./forked_file_reader path/to/input.txt
```

## Notes

- Supply the file path at runtime; recompilation is not needed to change inputs.

## Implementation approach

The reference implementation follows the [C11/POSIX pattern guide](../../notes/modern_c_patterns.md).
Keep `labs/common/` with this lab. From the repository root, `make all` builds
all labs and `make test` checks behavior and startup/shutdown failures.
