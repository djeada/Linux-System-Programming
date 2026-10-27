# Maze Navigator Challenge

## Objective

Implement a console-based maze navigation game in C. The player must move from the start (`S`) to the exit (`E`) using timed input, hints for invalid moves, and a cheat mode.

## What You Must Implement

1. **Command-line input**
   - The program accepts a single argument: the maze file path.
   - The file describes a rectangular maze with consistent row lengths.

2. **Maze parsing**
   - Load the maze into a 2D array.
   - Track which cells are hidden vs. revealed.
   - Validate that there is exactly one `S` and at least one `E`.

3. **Movement**
   - Use `W`, `A`, `S`, `D` (case-insensitive).
   - Reject moves into walls (`#`) or outside the maze.
   - After a valid move, reveal the current cell and its four neighbors.

4. **Invalid move hint**
   - If the move is invalid, reveal one adjacent unrevealed safe cell as a hint.

5. **Timed input**
   - Start a 5-second timer before each prompt.
   - If no input arrives within 5 seconds, print a reminder and keep the game running.
   - EOF exits cleanly; signal notifications still work with partial input.

6. **Cheat mode**
   - `Ctrl+Z` reveals one random safe hidden cell.

7. **Game end conditions**
   - Reaching `E` ends the game with a success message.
   - If the maze becomes fully revealed, the game ends and shows the maze.

## Maze File Format

- `#` wall
- `S` start (exactly one)
- `E` exit (at least one)
- Any other character is open path
- All rows must have the same length

## Hints

- Use two 2D arrays: one for the maze, one for revealed cells.
- Validate input early and fail fast if the file is malformed.
- Use `poll()` for the input timeout and a self-pipe to wake the main loop on a signal.
- Install handlers with `sigaction()`. Handlers set `sig_atomic_t` flags and notify the pipe; maze updates, random selection, and printing run in normal control flow.
- Clear the screen only for terminal output; redirected output stays readable.
- Keep your code modular: parsing, display, input, movement, hints, and signals should be separate functions.

## Build and Run

```sh
cc -std=c11 -Wall -Wextra -pedantic -o lab1 main.c -pthread
./lab1 maze.txt
```

## Implementation approach

The reference implementation follows the [C11/POSIX pattern guide](../../notes/modern_c_patterns.md).
Keep `labs/common/` with this lab. From the repository root, `make all` builds
all labs and `make test` checks behavior and startup/shutdown failures.
