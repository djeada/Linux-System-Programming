# 02 — printf() interprets your input

**Runtime:** 14 min · **Weakness:** CWE-134 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A log line supplied by a user is treated as formatting instructions instead of ordinary text.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build/02-format-string/vulnerable 'hello %p'
./security-videos/build/02-format-string/fixed 'hello %p'
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

`printf(user_text)` makes untrusted text the format string. Conversion sequences such as `%p` may consume absent arguments: that is undefined behavior and can disclose data or crash. The demo only uses harmless local input.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Use a constant format string: `printf("%s", user_text)`.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Compile with `-Wformat-security` and explain which warning indicates a trust-boundary violation.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
