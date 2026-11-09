# 13 — A password prefix passes authentication

**Runtime:** 13 min · **Weakness:** CWE-287 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

An empty string or password prefix is accepted by a flawed comparison.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build/13-prefix-auth/vulnerable sec
./security-videos/build/13-prefix-auth/fixed sec
```

## Why this is a vulnerability

`strncmp` compares only the number of characters provided by the user. Zero characters always compare equal, and short correct prefixes are not full credentials.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

For the toy demo, compare complete strings with `strcmp`; production authentication must use password hashing and appropriate verification.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Try an empty input and explain why no characters can be equal to a prefix of every string.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
