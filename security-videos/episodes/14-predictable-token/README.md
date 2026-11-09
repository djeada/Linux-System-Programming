# 14 — Your reset token is just a guessed seed

**Runtime:** 16 min · **Weakness:** CWE-338 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

Two processes given the same predictable seed generate an identical token.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build/14-predictable-token/vulnerable 1234
./security-videos/build/14-predictable-token/vulnerable 1234
./security-videos/build/14-predictable-token/fixed
```

## Why this is a vulnerability

`rand` is a deterministic PRNG, and `time(NULL)` is a low-entropy, observable seed. Even different calls in the same second often produce the same first output.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Request bytes from the Linux kernel's cryptographic RNG with `getrandom`, accounting for partial reads and interrupted calls.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Explain why a larger printed token is not necessarily more secure if its seed is guessable.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
