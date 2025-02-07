**Forked File Reader**

### Objective

The goal of this project is to gain hands-on experience with process creation in Unix-like systems by using the `fork()` system call. Your program will create a child process, and both the parent and child processes will independently read the same file line by line. As they read the file, each process will display each line along with its own process ID (PID) to the screen.

### Task Description

- **Process Creation:**  
  Your program should begin by calling `fork()` to create a child process. After forking, both the parent and child processes will execute the same code to read the file.

- **File Reading:**  
  - Open a specified file (the file path is hardcoded in your source code and must be updated to reflect the correct path on your machine).
  - Both the parent and the child processes will read the file line by line.
  - For every line that is read, print the line to the screen prefixed by the process's PID. For example:
    ```
    [PID 12345] This is a line from the file.
    ```

- **Process Coordination:**  
  - There is no need for explicit inter-process communication or synchronization for this task. It is expected that the outputs from the two processes may interleave.
  - Make sure that both processes are capable of reading the file independently. Consider what happens if both processes attempt to read from the same file descriptor concurrently.

- **Error Handling:**  
  - If the file cannot be opened (e.g., due to an incorrect path), your program should print an error message and exit gracefully.

---

### Compilation and Execution Instructions

1. **Compilation:**  
   - Use the default system compiler (such as `gcc`) without any special compile-time flags or command-line arguments.
   - Example command:  
     ```
     gcc your_program.c -o forked_file_reader
     ```

2. **Execution:**  
   - Simply run the executable. No command-line arguments are needed.
   - Example command:  
     ```
     ./forked_file_reader
     ```
   - **Important:** Make sure that the file path you have hardcoded in your program is correct for your machine. Adjust it if necessary.

### Additional Considerations

- **Output Interleaving:**  
  - Because both processes are reading from the file simultaneously, their outputs may interleave on the terminal. This is acceptable and part of the challenge of understanding process scheduling.

- **Code Documentation:**  
  - Comment your code thoroughly to explain the use of `fork()`, file I/O operations, and the handling of process IDs.

- **Testing:**  
  - Test your program on your machine to ensure that both processes correctly read the file and print the expected output.

- **Submission:**  
  - Submit your source code file along with a brief README file that includes any special instructions or observations you made during development.
