# 06 — A deleted contact still has a pointer

**Runtime:** 17 min · **Weakness:** CWE-416 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

After deleting a contact, a stale pointer is dereferenced; AddressSanitizer identifies use-after-free.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build-asan/06-use-after-free/vulnerable Ada
./security-videos/build/06-use-after-free/fixed Ada
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

`free` ends the allocation lifetime. Storing the same address in a pointer does not preserve object ownership.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Read data before freeing, then stop using the object and clear stale references; discuss multiple aliases.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Create two aliases for the contact and explain why setting only one alias to NULL does not make the other safe.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
