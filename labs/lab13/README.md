# Lab 13: Pipelines and terminal job control

Keep lab3 as a small `posix_spawnp` launcher. This separate shell studies the
custom process-group and terminal setup around `fork`/`exec`. Read
[shell and terminal notes](../../notes/shells_and_terminals.md).

```sh
make build/lab13
./build/lab13
```

Try these commands **inside** the lab shell:

```sh
printf 'hello world\n' | tr a-z A-Z > output.txt
cat < output.txt
printf 'another line\n' >> output.txt
sleep 30
# Press Ctrl+Z, then enter:
jobs
bg 1
fg 1
# Press Ctrl+C, then enter:
exit 0
```

Supported syntax: whitespace-separated arguments, single/double quotes,
backslash escapes outside single quotes, `|`, `<`, `>`, `>>`, and a final `&`.
There is no variable/glob/command expansion, `;`, `&&`, here-document, or full
POSIX shell grammar. Backslash inside double quotes escapes the next character
in this simplified grammar. Limits are 64 KiB per line, 256 tokens, 8 pipeline
stages, 64 arguments per command, and 16 jobs. Invalid syntax is rejected.

`cd`, `exit [0..255]`, `jobs`, `fg N`, and `bg N` run in the parent and must
stand alone without redirection/background execution. A pipeline's status is
its last stage's status. New output files are private `0600` subject to umask.
Batch mode accepts commands on stdin; background commands default to `/dev/null`
for input. Interactive mode requires a controlling terminal.

Each pipeline has one process group. A startup gate holds children until the
parent registers them and gives a foreground group the terminal. Unused pipe
ends close in every process. The shell reaps all members, handles stopped jobs,
and reclaims terminal ownership and settings. On exit it signals its remaining
groups with SIGHUP/SIGCONT, waits briefly, then kills/reaps remaining members.
Children restore default terminal-signal dispositions before exec.

**Fragile recipe:** wait for the first stage before launching the next, retain
a pipe writer indefinitely, or send terminal signals to one PID instead of the
job's group. Tests exercise large pipelines, failed partial startup, and actual
Ctrl+Z/bg/fg/Ctrl+C behavior through a pseudo-terminal.
