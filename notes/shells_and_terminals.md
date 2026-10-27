# Shells, process groups, and terminals

Companion: [lab13](../labs/lab13/README.md). [Lab3](../labs/lab3/README.md)
remains the smaller launch example. The choice of API depends on the setup
needed; `fork` is a current API, not a synonym for outdated code.

## Pipeline ownership

Create all pipes before launching stages, then launch all stages before waiting.
A pipe has finite capacity: waiting for a producer before starting its consumer
can deadlock. In each child, use `dup2` to select stdin/stdout and close every
unused pipe end. In the parent, close its copies after startup. A leftover
writer can prevent the reader from seeing EOF. Set CLOEXEC at creation as an
additional inheritance boundary.

Redirections are explicit descriptor operations before exec. Builtins such as
`cd` must run in the shell process to change its directory. Parsing, expansion,
descriptor setup, executable lookup, and child supervision are distinct jobs.
Lab13 documents its small grammar rather than pretending to implement all
POSIX shell language features.

## A terminal controls a group

A pipeline is a process group; a session can contain several groups. The
controlling terminal has a foreground process group. Terminal-generated
Ctrl+C/Ctrl+Z reach that group, enabling a whole pipeline to be interrupted or
stopped together. A background process reading the terminal may receive
SIGTTIN. The shell changes foreground ownership using `tcsetpgrp` and restores
its own ownership/settings after a foreground job stops or exits. See
[credentials(7)](https://man7.org/linux/man-pages/man7/credentials.7.html) and
[tcsetpgrp(3)](https://man7.org/linux/man-pages/man3/tcsetpgrp.3.html).

Both parent and child call `setpgid` to close startup races. Lab13 holds children
behind a pipe gate until the parent has registered the complete job and given
it the terminal. Without that ordering, a fast foreground command can attempt
input before its group owns the terminal and be stopped accidentally.

The shell ignores the terminal signals it manages, while children restore
default dispositions before exec. A blocked SIGCHLD is consumed via signalfd
in normal parent code. `waitpid` with stopped/continued status tracks every
pipeline member. Do not free the job when only one member exits. `jobs` shows
state, `bg` resumes a stopped group, and `fg` transfers the terminal and resumes
it. Stopped job terminal modes are saved for later foreground continuation.

## Scope and cleanup

Lab13 is single-threaded, so its post-fork setup may use routines that would
need stricter treatment after fork from a multithreaded program. Its ordinary
exit owns and terminates remaining groups, then reaps its children. Jobs that
deliberately create new sessions/groups escape this simple ownership model;
a production supervisor can use stronger containment. Forced SIGKILL cannot
run cleanup. Batch backgrounds use `/dev/null` by default so they cannot consume
the shell's command input.

Compare the fork setup with `posix_spawn` file actions and process-group
attributes as an exercise. The terminal transfer and complete job-state
protocol still need design with either launch API.
