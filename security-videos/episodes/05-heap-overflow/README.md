# 05 — malloc() does not enforce your limits

**Runtime:** 17 min · **Weakness:** CWE-122 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A short `malloc` followed by an unlimited copy triggers a heap-buffer-overflow diagnostic.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build-asan/05-heap-overflow/vulnerable AAAAAAAAAAAAAAAAAAAAAAAAAAAAA
./security-videos/build/05-heap-overflow/fixed AAAAAAAAAAAAAAAAAAAAAAAAAAAAA
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

`malloc(16)` gives exactly sixteen bytes; `strcpy` does not know that capacity and may write into adjacent heap memory.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Reject overlong messages before copying, include space for the terminator, and check allocation errors.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Modify the fixed version to dynamically allocate `strlen(input)+1` with a checked size calculation.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
