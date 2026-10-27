# Service lifecycle and systemd

Companion: [lab14](../labs/lab14/README.md).

## Startup, operation, and shutdown

A service is ready when required resources and configuration are usable, not
merely when its PID exists. Keep startup failure distinct from a successful
ready state. Lab14 loads and validates configuration, installs its signal/timer
descriptors, then sends readiness. Under a service manager, stay in the
foreground and let the manager own restart policy, output collection, and
process tracking. Double-fork daemonization is historical context for another
launch environment, not a requirement of a managed service.

`signalfd` requires the corresponding signals to be blocked. In a threaded
service block them before creating workers so they inherit the mask. Lab14 is
single-threaded and handles SIGHUP/SIGINT/SIGTERM in its normal `poll` loop.
No asynchronous handler performs allocation, stdio, or configuration updates.
`timerfd` uses a monotonic clock and exposes coalesced expirations explicitly.

For reload, parse into a candidate object and validate before publishing it.
A malformed reload leaves the active configuration intact. For shutdown,
stop accepting new work, finish or cancel owned work according to a deadline,
release resources, and exit with a meaningful status. This lab's work is
synchronous logging, so there is no worker queue to drain.

## Manager integration

The optional unit uses `Type=notify`; the process sends a datagram to
`NOTIFY_SOCKET` containing `READY=1` after initialization. The sample also
sends `STATUS=...` and `STOPPING=1`. Use the documented
[systemd notification protocol](https://www.freedesktop.org/software/systemd/man/latest/sd_notify.html)
or libsystemd for more involved integrations. A PID that exits immediately
after sending a notification can introduce attribution races; a long-lived
main service avoids that particular pattern. Watchdog operation, fd storage,
socket activation, and `Type=notify-reload` are separate extensions.

The example user unit's `ExecReload` sends SIGHUP; that command succeeding
does not acknowledge completed asynchronous reload. Logs show accepted or
rejected reloads. `Restart=on-failure` lets the manager restart actual failures;
graceful termination returns success. Keep logs useful for operators, and
avoid embedding secrets in messages. A production unit should define resource
and access restrictions based on the real workload rather than copying a
generic hardening list blindly.

Exercises: observe ordering between READY and the first tick; reload an invalid
interval; stop during normal work; add a bounded worker queue and an explicit
drain deadline. Tests emulate the manager with a local datagram receiver and
never install or start a real service.
