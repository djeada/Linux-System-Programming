**Build Your Own Shell**

**Objective**  
Create a minimal Unix-style shell in C that reads commands, parses arguments, and executes programs using `fork()` and `execvp()`.

**What You Must Implement**

1. **Prompt loop**  
   - Continuously display a prompt (e.g., `myshell> `).
   - Read a full line of user input each iteration.

2. **Command parsing**  
   - Tokenize the input line into a command and its arguments.
   - Split on whitespace and build a NULL-terminated `argv` array.

3. **Process execution**  
   - Use `fork()` to create a child process.
   - In the child, call `execvp()` with the parsed arguments.
   - In the parent, wait for the child to finish before prompting again.

4. **Exit handling**  
   - If the user types `exit`, terminate the shell cleanly.

5. **Error handling**  
   - Print clear errors when `fork()` or `execvp()` fails.
   - Handle empty input lines without crashing.

**Hints**

- Strip the newline from `fgets()` input before tokenizing.
- Keep parsing and execution in separate functions for clarity.
- Use `waitpid()` in the parent to avoid zombie processes.
- Do not require a trailing space in user commands.

**Build and Run**

```sh
cc -std=c11 -Wall -Wextra -pedantic -o myshell main.c
./myshell
```

**Optional Extensions**

- Implement built-ins such as `cd` or `help`.
- Add basic signal handling so `Ctrl+C` interrupts running commands but not the shell.
