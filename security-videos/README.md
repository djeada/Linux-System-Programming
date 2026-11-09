# Code → Break → Fix: C Security Video Track

Fifteen **10–20 minute, no-introduction** episodes for cybersecurity students, built from small intentionally vulnerable Linux/C programs. Each episode has a runnable `vulnerable.c`, a `fixed.c`, and a short shooting script explaining **the bug, the visible impact, the mechanism, the exact fix and a student challenge**.

> **Authorized local labs only.** Do not run the deliberately unsafe binaries as root or against real accounts, files, networks or systems. Use disposable VMs/containers and dummy input. Some examples invoke undefined behavior; exploit outcomes are NOT guaranteed on every compiler or optimization level. Sanitizer reports are evidence of memory errors, not proof of arbitrary code execution.

## Build & run

From repository root:

```sh
make -C security-videos all
make -C security-videos test
make -C security-videos asan    # opt-in: instrument memory-unsafe examples
```

Run a sample (from repo root):

```sh
./security-videos/build/13-prefix-auth/vulnerable sec
./security-videos/build/13-prefix-auth/fixed sec
```

For tests using paths relative to the working directory (04 and 15), run from a **temporary disposable directory**, following their episode READMEs. The Python smoke tests do this automatically.

## Video sequence

| # | 10–20 minute episode | CWE | Runtime |
| --- | --- | --- | --- |
| [01](episodes/01-admin-flag-overwrite/README.md) | One strcpy() and you're admin | CWE-787 | 18 min |
| [02](episodes/02-format-string/README.md) | printf() interprets your input | CWE-134 | 14 min |
| [03](episodes/03-command-injection/README.md) | The filename that becomes a shell command | CWE-78 | 18 min |
| [04](episodes/04-path-traversal/README.md) | The note viewer escapes its directory | CWE-22 | 16 min |
| [05](episodes/05-heap-overflow/README.md) | malloc() does not enforce your limits | CWE-122 | 17 min |
| [06](episodes/06-use-after-free/README.md) | A deleted contact still has a pointer | CWE-416 | 17 min |
| [07](episodes/07-double-free/README.md) | Two cleanup paths, one allocation | CWE-415 | 13 min |
| [08](episodes/08-integer-overflow/README.md) | Wrapped allocation size, enormous write | CWE-190 | 19 min |
| [09](episodes/09-negative-length/README.md) | Negative input becomes a huge memcpy | CWE-195 | 16 min |
| [10](episodes/10-off-by-one/README.md) | One extra index past the end | CWE-193 | 13 min |
| [11](episodes/11-unterminated-string/README.md) | strncpy() is not always a safe string | CWE-170 | 14 min |
| [12](episodes/12-packet-length/README.md) | The packet says 200 bytes, but only 3 arrived | CWE-125 | 19 min |
| [13](episodes/13-prefix-auth/README.md) | A password prefix passes authentication | CWE-287 | 13 min |
| [14](episodes/14-predictable-token/README.md) | Your reset token is just a guessed seed | CWE-338 | 16 min |
| [15](episodes/15-temp-file-symlink/README.md) | A temporary report clobbers another file | CWE-59 | 18 min |

## Teaching format

1. **0:00–1:00: Show the effect.** Start with the locally controlled failure or unauthorized result. Avoid introductions.
2. **1:00–5:00: Type the wrong C program.** Identify the precise unsafe API, incorrect length, broken ownership, or misplaced trust assumption.
3. **5:00–10:00: Trigger and inspect.** Use a harmless lab input and, for memory bugs, GDB or AddressSanitizer. Explain the data/control boundary and the limits of the observation.
4. **10:00–15:00: Make one real fix.** Edit code to remove the cause, not merely suppress crashes.
5. **Last 2–4 minutes: Repeat the input and challenge viewers.** Demonstrate that the original failing input is rejected, inert, or safely handled; assign an edge-case regression test.

The timestamp split adapts to each episode's specified runtime. Keep input, before/after output and code diff on screen rather than spending time on language basics.

## Compiler notes

The ordinary `all` target builds with `-O0 -g -Wall -Wextra -Wpedantic`. Intentionally broken examples may generate warnings. `asan` builds separate instrumented binaries with `-fsanitize=address,undefined`. A sanitizer process often exits nonzero by design. Avoid treating a plain crash as proof of exploitability. These examples prioritize understanding the *mechanism*; they are not production software.

Related material in this repository: [Linux security and isolation](../notes/security_and_isolation.md), [memory management](../notes/memory_management.md), [diagnostics lab](../labs/lab16/README.md), [modern C/POSIX patterns](../notes/modern_c_patterns.md).
