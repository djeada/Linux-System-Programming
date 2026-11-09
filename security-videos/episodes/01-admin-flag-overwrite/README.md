# 01 — One strcpy() and you're admin

**Runtime:** 18 min · **Weakness:** CWE-787 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

Pass a username that is too long; the toy account's adjacent admin flag changes without authentication.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build/01-admin-flag-overwrite/vulnerable AAAAAAAAA
./security-videos/build/01-admin-flag-overwrite/fixed AAAAAAAAA
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

`strcpy` copies until a NUL byte without checking the eight-byte destination. In a common lab layout, later bytes enter the next struct member. Because this is undefined behavior, the outcome is compiler/build dependent; inspecting `offsetof` and the sanitizer output matters.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Reject usernames that cannot fit INCLUDING the NUL terminator, then copy the checked number of bytes.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Change the buffer length and optimize with `-O2`; explain any change in observed behavior.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
