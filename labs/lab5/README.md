# Three-Process Text Transformation Pipeline

## Objective

Build a three-stage pipeline using **processes** and **shared memory**. Each stage transforms text and passes it to the next stage through a shared buffer with proper synchronization.

## Concepts Covered

- Process creation and coordination
- Shared memory (`mmap`) across processes
- Producer/consumer synchronization
- POSIX semaphores
- Stream-style text transformation

## C APIs You Will Likely Use

- `fork`, `waitpid`
- `mmap`, `munmap`
- `sem_init`, `sem_wait`, `sem_post`, `sem_destroy`
- `fopen`, `fgetc`, `fclose`, `printf`

## What You Must Implement

1. **Reader process (producer)**
   - Reads an input file character by character.
   - Replaces each newline (`\n`) with the literal marker `"<EOL>"`.
   - Writes the transformed stream into the first shared buffer.

2. **Transformer process (squasher)**
   - Reads from the first shared buffer.
   - Replaces every `"**"` with a single `"#"`.
   - Writes the transformed stream into the second shared buffer.

3. **Printer process (formatter)**
   - Reads from the second shared buffer.
   - Prints output in lines of exactly **20 characters** each (configurable via CLI).

## Synchronization Rules

- Use shared memory for buffers.
- Use semaphores (or another proper synchronization method) so producers and consumers do **not** busy-wait.
- Each buffer should track when the producer is done so the consumer can exit cleanly.

## Example

Input file (`input.txt`):
```
A**
B
```
Reader output stream (conceptually):
```
A**<EOL>B<EOL>
```
Transformer output stream:
```
A#<EOL>B<EOL>
```
Printer output (20-char lines; last line may be shorter):
```
A#<EOL>B<EOL>
```

## Constraints and Edge Cases

- Do not busy-wait; block on semaphores.
- Use fixed-capacity shared ring buffers to provide backpressure.
- Preserve a non-star character after a pending `*`; preserve a trailing single `*` at EOF.
- If one stage fails, stop and reap its peers, then return failure.
- Ensure consumers terminate cleanly when producers finish.
- Line width must be a positive integer.

## Deliverables

- `main.c` (your solution)
- `README.md` (brief build/run notes)
- `input.txt` (or pass a custom file path)

## Build and Run

```sh
cc -std=c11 -Wall -Wextra -pedantic -o pipeline main.c -pthread
./pipeline [input_file] [line_width]
```

Example:

```sh
./pipeline input.txt 20
```

## Notes

- Defaults: `input.txt` and line width `20`.

## Implementation approach

The reference implementation follows the [C11/POSIX pattern guide](../../notes/modern_c_patterns.md).
Keep `labs/common/` with this lab. From the repository root, `make all` builds
all labs and `make test` checks behavior and startup/shutdown failures.
