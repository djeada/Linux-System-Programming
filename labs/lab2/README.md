**Forked File Reader**

**Objective**  
Practice process creation with `fork()` by having a parent and child read the same file independently and print each line with their PID.

**What You Must Implement**

1. **Process creation**  
   - Call `fork()` once to create a child process.
   - Both parent and child should execute the same reading logic.

2. **File reading**  
   - Open a file whose path is hardcoded in your source (update the macro before compiling).
   - Read the file line by line using standard I/O.
   - Print each line prefixed with the process ID, e.g.:
     ```
     [PID 12345] This is a line from the file.
     ```

3. **Error handling**  
   - If `fork()` or `fopen()` fails, print an error message and exit.

**Hints**

- Each process should open the file independently after `fork()`.
- Output from the two processes may interleave; that is expected.
- Keep the program simple: no synchronization or IPC is required.

**Build and Run**

```sh
cc -std=c11 -Wall -Wextra -pedantic -o forked_file_reader main.c
./forked_file_reader
```

**Notes**

- Update `FILE_PATH` in `main.c` before compiling.
