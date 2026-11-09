# 15 — A temporary report clobbers another file

**Runtime:** 18 min · **Weakness:** CWE-59 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A predictable report filename is pre-created as a symlink to a disposable file; the writer follows it.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
# Run these from the repository root, in a disposable directory:
make -C security-videos all
BIN="$(pwd)/security-videos/build/15-temp-file-symlink"
WORK="$(mktemp -d)"
(
  cd "$WORK"
  mkdir scratch
  printf 'ORIGINAL\n' > victim.txt
  ln -s ../victim.txt scratch/report.tmp
  "$BIN/vulnerable"                    # follows symlink
  cat victim.txt                        # REPORT (overwritten)
  printf 'ORIGINAL\n' > victim.txt
  "$BIN/fixed"                         # rejects existing symlink
  cat victim.txt                        # ORIGINAL (preserved)
)
rm -rf "$WORK"
```

**Workspace warning:** The commands create only dummy files inside `mktemp -d` and clean them up. Never substitute real directories or real secrets.

## Why this is a vulnerability

Opening a predictable writable path with `fopen(..., "w")` follows a symlink and truncates its target. The demo uses a local `scratch/` directory, NEVER `/tmp` or real files.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Open a trusted directory, create a NEW file with `O_CREAT|O_EXCL|O_NOFOLLOW` and restrictive permissions.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Explain how to generate unpredictable temporary names safely with `mkstemp` for real applications.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
