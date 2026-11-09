# 07 — Two cleanup paths, one allocation

**Runtime:** 13 min · **Weakness:** CWE-415 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

The second `free()` operates on memory already released, and a sanitizer detects it.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build-asan/07-double-free/vulnerable
./security-videos/build/07-double-free/fixed
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

Two paths assume they each own the same allocation. Calling `free` on an already freed non-NULL pointer is undefined behavior.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Release each allocation only once; centralize cleanup and clarify ownership.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Describe an ownership strategy that stays correct even when two structs reference the same allocation.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
