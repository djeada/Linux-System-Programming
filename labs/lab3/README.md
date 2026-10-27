# Build Your Own Shell

## Objective

Create a minimal Unix-style shell in C that reads commands, parses arguments, and executes programs using `posix_spawnp()` and checked child completion. A separate fork/exec implementation exposes the lower-level launch mechanics for comparison.

## What You Must Implement

1. **Prompt loop**
   - Continuously display a prompt (e.g., `myshell> `).
   - Read a full line of user input each iteration.

2. **Command parsing**
   - Tokenize the input line into a command and its arguments.
   - Split on whitespace and build a NULL-terminated `argv` array.

3. **Process execution**
   - Use `posix_spawnp` for a direct executable launch and PATH search.
   - Check its returned error number.
   - Wait for the child with `waitpid`, retry interrupted waits, and inspect its exit status.

4. **Exit handling**
   - If the user types `exit`, terminate the shell cleanly.

5. **Error handling**
   - Report launch errors separately from a child program that exits unsuccessfully.
   - Handle empty input lines without crashing.

## Hints

- Read complete lines with `getline`; grow the argument vector instead of truncating commands.
- Keep parsing and execution in separate functions for clarity.
- Use `waitpid()` in the parent to avoid zombie processes.
- Do not require a trailing space in user commands.

## Build and Run

```sh
cc -std=c11 -Wall -Wextra -pedantic -o myshell main.c -pthread
./myshell
```

## Fork/exec API comparison

`comparisons/fork_exec.c` shares the same parser and uses explicit `fork` and
`execvp`. A failed exec reports through the child exit status and uses `_exit`
rather than inherited parent cleanup. Fork/exec remains appropriate for custom
child setup; it is not a deprecated API.

```sh
cc -std=c11 -Wall -Wextra -pedantic comparisons/fork_exec.c -o fork_exec_shell -pthread
./fork_exec_shell
```

This minimal shell splits on whitespace. It does not implement quoting, pipes,
redirection, expansion, or job control. EOF exits cleanly.

## Optional Extensions

- Implement built-ins such as `cd` or `help`.
- Add basic signal handling so `Ctrl+C` interrupts running commands but not the shell.

## Implementation approach

The reference implementation follows the [C11/POSIX pattern guide](../../notes/modern_c_patterns.md).
Keep `labs/common/` with this lab. From the repository root, `make all` builds
all labs and `make test` checks behavior and startup/shutdown failures.
