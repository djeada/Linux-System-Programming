# 11 — strncpy() is not always a safe string

**Runtime:** 14 min · **Weakness:** CWE-170 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A username that fills its destination may not have a terminating NUL, causing a read past the array.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build-asan/11-unterminated-string/vulnerable ABCDEFGH
./security-videos/build/11-unterminated-string/fixed ABCDEFGH
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

`strncpy` does not append `\0` if the source is as long as the count. `%s` then reads until it finds a NUL elsewhere.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Reject input that cannot fit with its terminator and copy the checked length plus one.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Explain why `strncpy(dest, input, sizeof dest)` is not a universal repair for `strcpy`.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
