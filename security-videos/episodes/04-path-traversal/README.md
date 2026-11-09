# 04 — The note viewer escapes its directory

**Runtime:** 16 min · **Weakness:** CWE-22 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A request for `../private.txt` reads a lab-only file that was supposed to be inaccessible.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
cd security-videos/episodes/04-path-traversal
mkdir -p notes
printf 'PUBLIC\n' > notes/hello.txt
printf 'PRIVATE\n' > private.txt
../../build/04-path-traversal/vulnerable '../private.txt'
../../build/04-path-traversal/fixed '../private.txt'
```

**Workspace warning:** The demo writes only disposable files in the lesson directory (or a temporary copy of it); it should not be run from the repository root. Clean up the sample files afterwards. For a fully isolated run, use a fresh temporary working directory as in the automated tests.

## Why this is a vulnerability

Combining `notes/` with an untrusted path does not constrain filesystem resolution. The `..` component crosses the intended directory boundary.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Open a trusted directory and use `openat` on a single safe filename with `O_NOFOLLOW`, rejecting separators and dot components.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Add a symlink named `notes/link` pointing outside `notes/`; explain why `O_NOFOLLOW` matters.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
