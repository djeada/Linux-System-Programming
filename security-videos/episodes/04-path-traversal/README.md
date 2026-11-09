# 04 — The note viewer escapes its directory

**Runtime:** 16 min · **Weakness:** CWE-22 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A request for `../private.txt` reads a lab-only file that was supposed to be inaccessible.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
# Run these from the repository root, in a disposable directory:
make -C security-videos all
BIN="$(pwd)/security-videos/build/04-path-traversal"
WORK="$(mktemp -d)"
(
  cd "$WORK"
  mkdir notes
  printf 'PUBLIC\n' > notes/hello.txt
  printf 'PRIVATE\n' > private.txt
  "$BIN/vulnerable" '../private.txt'  # reads PRIVATE (bug)
  "$BIN/fixed" '../private.txt'       # rejects traversal
  "$BIN/fixed" hello.txt              # reads PUBLIC
)
rm -rf "$WORK"
```

**Workspace warning:** The commands create only dummy files inside `mktemp -d` and clean them up. Never substitute real directories or real secrets.

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
