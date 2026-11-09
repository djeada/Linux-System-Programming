# 10 — One extra index past the end

**Runtime:** 13 min · **Weakness:** CWE-193 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

The loop's last iteration writes one byte beyond a 16-byte stack array.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build-asan/10-off-by-one/vulnerable
./security-videos/build/10-off-by-one/fixed
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

Valid indices are 0..15, not 0..16. A single extra write is undefined behavior; this toy demonstrates memory corruption rather than guaranteed code execution.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Use a strict `<` bound and derive it from the destination capacity.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Replace the fixed loop with a checked `memset` and explain whether the bounds remain correct.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
