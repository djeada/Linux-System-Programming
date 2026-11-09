# 03 — The filename that becomes a shell command

**Runtime:** 18 min · **Weakness:** CWE-78 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A diagnostic tool that merely checks a filename runs an unintended marker command.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build/03-command-injection/vulnerable 'missing; echo INJECTED; #'
./security-videos/build/03-command-injection/fixed 'missing; echo INJECTED; #' 
```

## Why this is a vulnerability

`system` executes a shell. Shell metacharacters from user input are reinterpreted as command separators, not filename characters.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Use `stat()` directly for this file-existence operation, removing the shell as an interpreter.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Explain why quoting a user string manually is less robust than using a direct OS API.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
