# 09 — Negative input becomes a huge memcpy

**Runtime:** 16 min · **Weakness:** CWE-195 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

The input `-1` passes a maximum-length check, then becomes a huge unsigned copy size.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build-asan/09-negative-length/vulnerable -1
./security-videos/build/09-negative-length/fixed -1
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

The code checks only `length > 32`; when `memcpy` receives the negative `int`, it converts to the unsigned `size_t` parameter.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Parse with `strtol`, reject invalid or negative values, validate all bounds before converting to `size_t`.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Try `0`, `32`, `33`, and nonnumeric input against the fixed version.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
