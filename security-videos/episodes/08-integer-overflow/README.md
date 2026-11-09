# 08 — Wrapped allocation size, enormous write

**Runtime:** 19 min · **Weakness:** CWE-190 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A record count makes a 16-bit byte count wrap, so allocation is smaller than the data written.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build-asan/08-integer-overflow/vulnerable 4100
./security-videos/build/08-integer-overflow/fixed 4100
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

Casting the product to `uint16_t` loses high bits. The subsequent write uses the original count and exceeds the allocation.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Check multiplication in `size_t`, bound resource use, and use the validated result consistently.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Explain why checking an overflowed result after multiplication is too late.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
