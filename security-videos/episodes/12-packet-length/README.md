# 12 — The packet says 200 bytes, but only 3 arrived

**Runtime:** 19 min · **Weakness:** CWE-125 · **Level:** hands-on C security · **Prerequisite:** can compile and run a C file

## 0:00 — Open with the exploit

A toy binary packet claims a payload longer than the received buffer and the parser reads beyond its end.

## From an empty editor: code the bug

Type [vulnerable.c](vulnerable.c) line by line. Build with `make -C security-videos all` from the repository root. Name the exact wrong assumption before demonstrating it.

## Trigger the effect

```sh
./security-videos/build-asan/12-packet-length/vulnerable 200
./security-videos/build/12-packet-length/fixed 200
```

For observable memory errors, first run `make -C security-videos asan` and use the instrumented `build-asan` executable. Expect possible nonzero exit; sanitizer diagnostics, not arbitrary takeover, are the intended evidence.

## Why this is a vulnerability

The length byte is untrusted metadata, not evidence that the corresponding bytes are present. Out-of-bounds reads can disclose memory or crash a real parser.

**Trust boundary:** untrusted command-line input or program state crosses into an unchecked memory operation, security decision, shell, or filesystem operation. Point to the exact line and identify the violated assumption.

## Fix and retest

Type [fixed.c](fixed.c), explaining each changed line.

Compare the declared length against both received bytes and output capacity before copying.

Repeat the **same input** against the fixed binary. Discuss what changed and what the fix guarantees. See the [series guide](../../README.md) for the common filming format and build instructions.

## Student challenge

Make the parser accept a caller-provided actual packet length and add tests for truncated headers.

**Safety:** use toy data, a disposable non-root lab, and only these local binaries; never use real secrets or privileged paths.
