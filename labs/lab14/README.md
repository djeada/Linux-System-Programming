# Lab 14: Foreground service lifecycle

Implement readiness, periodic work, configuration reload, and orderly shutdown.
See [service notes](../../notes/service_lifecycle.md).

```sh
make build/lab14
./build/lab14 labs/lab14/service.conf
# In another terminal, using the PID printed by your shell/ps:
kill -HUP PID
kill -TERM PID
```

Arguments are `config_file [max_ticks]`; omit the tick limit to run until
SIGINT/SIGTERM. Configuration accepts `interval_ms=positive_integer` and
`message=text`, once each, plus blank/comment lines. Defaults are 1000 ms and
`hello`. Unknown/duplicate/malformed fields reject a configuration.

`signalfd`, `timerfd(CLOCK_MONOTONIC)`, and `poll` put signals, reload, timers,
and logging in normal code. Reload validates a candidate before replacing the
active configuration. Rejected reloads retain the previous settings. Logs go
to stdout/stderr; each timer wake includes its coalesced expiration count.

When `NOTIFY_SOCKET` is set, the program sends systemd's `READY=1` after startup
and `STOPPING=1` at shutdown; both pathname and Linux abstract sockets work.
No libsystemd dependency is required for this small protocol exercise.
It does not implement watchdog keepalives, socket activation, or asynchronous
systemd reload acknowledgment.

`linux-lab.service` is an optional **user service** template using `Type=notify`.
Review its paths for your checkout before copying it into
`~/.config/systemd/user/` and running `systemctl --user daemon-reload` and
`systemctl --user start linux-lab.service`. Inspect logs with
`journalctl --user -u linux-lab.service`; stop it with `systemctl --user stop`.
Nothing in the build/tests installs or starts a unit.

**Current pattern:** stay in the foreground under a service manager, report
readiness after initialization, and finish owned work on termination.
**Historical comparison:** double-fork daemonization and handwritten PID files
solve a different launch environment and are unnecessary under this unit.
