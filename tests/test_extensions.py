"""Behavioral checks for file, network, terminal, service and Linux API labs."""
import concurrent.futures
import os
from pathlib import Path
import pty
import re
import selectors
import shlex
import shutil
import signal
import socket
import struct
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Extensions(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="linux-extensions-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.build = Path(cls.temporary.name)
        cls.cc = shlex.split(os.environ.get("CC", "cc"))
        cls.flags = shlex.split(os.environ.get(
            "LAB_TEST_CFLAGS", "-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror"))
        for number in range(9, 20):
            cls.compile(ROOT / f"labs/lab{number}/main.c", cls.build / f"lab{number}", "-pthread", "-ldl")
        cls.compile(ROOT / "labs/lab18/plugin.c", cls.build / "plugin.so", "-shared", "-fPIC")
        cls.compile(ROOT / "labs/lab17/comparisons/publication.c", cls.build / "publication", "-pthread")
        for fixture in ("failures", "io_failures"):
            subprocess.run([*cls.cc, "-std=c11", "-O2", "-shared", "-fPIC",
                            str(ROOT / f"tests/fixtures/{fixture}.c"), "-ldl", "-pthread",
                            "-o", str(cls.build / f"{fixture}.so")], check=True, capture_output=True)
        cls.uring_available = bool(shutil.which("pkg-config")) and subprocess.run(
            ["pkg-config", "--exists", "liburing"], capture_output=True).returncode == 0
        if cls.uring_available:
            uring_flags = shlex.split(subprocess.check_output(
                ["pkg-config", "--cflags", "--libs", "liburing"], text=True))
            cls.compile(ROOT / "labs/lab20/main.c", cls.build / "lab20", "-pthread", *uring_flags)

    @classmethod
    def compile(cls, source, output, *extra):
        result = subprocess.run([*cls.cc, *cls.flags, str(source), "-o", str(output), *extra],
                                capture_output=True, text=True)
        if result.returncode:
            raise AssertionError(result.stderr)

    def setUp(self):
        self.temporary_case = tempfile.TemporaryDirectory(prefix="linux-api-case-")
        self.addCleanup(self.temporary_case.cleanup)
        self.path = Path(self.temporary_case.name)

    def file(self, name, data):
        path = self.path / name
        path.write_bytes(data.encode() if isinstance(data, str) else data)
        return path

    def start(self, lab, args=(), env=None, stdin=subprocess.PIPE):
        process = subprocess.Popen([str(self.build / lab), *map(str, args)], cwd=self.path,
                                   env={**os.environ, **(env or {})}, stdin=stdin,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   start_new_session=True, bufsize=0)
        self.addCleanup(self.stop, process)
        return process

    @staticmethod
    def stop(process):
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait(timeout=3)
        for stream in (process.stdin, process.stdout, process.stderr):
            if stream and not stream.closed:
                stream.close()

    def run_lab(self, lab, args=(), data=b"", env=None, expected=0, timeout=5):
        process = self.start(lab, args, env)
        output, errors = process.communicate(data, timeout=timeout)
        if expected is None:
            self.assertNotEqual(process.returncode, 0, output)
        else:
            self.assertEqual(process.returncode, expected, errors.decode(errors="replace"))
        return output, errors

    def read_until(self, fd, needle, timeout=4):
        output = b""
        deadline = time.monotonic() + timeout
        with selectors.DefaultSelector() as ready:
            ready.register(fd, selectors.EVENT_READ)
            while needle not in output:
                remaining = deadline - time.monotonic()
                if remaining <= 0 or not ready.select(remaining):
                    self.fail(f"Waiting for {needle!r}, received {output!r}")
                chunk = os.read(fd if isinstance(fd, int) else fd.fileno(), 8192)
                self.assertTrue(chunk, output)
                output += chunk
        return output

    @staticmethod
    def receive_exact(peer, size):
        data = b""
        while len(data) < size:
            chunk = peer.recv(size - len(data))
            if not chunk:
                raise AssertionError("Truncated echo")
            data += chunk
        return data

    def echo(self, port, message, fragments=False):
        with socket.create_connection(("127.0.0.1", port), timeout=3) as peer:
            packet = struct.pack("!I", len(message)) + message
            if fragments:
                for part in (packet[:1], packet[1:3], packet[3:8], packet[8:]):
                    peer.sendall(part)
            else:
                peer.sendall(packet)
            peer.shutdown(socket.SHUT_WR)
            size = struct.unpack("!I", self.receive_exact(peer, 4))[0]
            self.assertEqual(size, len(message))
            self.assertEqual(self.receive_exact(peer, size), message)

    def server(self, lab, args):
        process = self.start(lab, args)
        ready = self.read_until(process.stdout, b"\n")
        match = re.search(rb"port=(\d+)", ready)
        return process, int(match[1]) if match else None

    def test_descriptor_copy_short_io_ranges_and_aliases(self):
        content = bytes(range(256)) * 100
        source = self.file("source", content)
        target = self.path / "copy"
        env = {"LD_PRELOAD": str(self.build / "io_failures.so"), "LAB_SHORT_IO": "1"}
        self.run_lab("lab9", [source, target], env=env)
        self.assertEqual(target.read_bytes(), content)
        output, _ = self.run_lab("lab9", [source, target, 17, 1001], env=env)
        self.assertEqual(target.read_bytes(), content[17:1018])
        self.assertIn(b"source_offset=0 destination_offset=0", output)
        alias = self.path / "alias"
        os.link(source, alias)
        self.run_lab("lab9", [source, alias], expected=None)
        self.assertEqual(source.read_bytes(), content)
        self.run_lab("lab9", [source, target, "-1", 4], expected=None)

    def test_filesystem_traversal_links_and_replacement(self):
        tree = self.path / "tree"
        (tree / "child").mkdir(parents=True)
        (tree / "child/file").write_text("contents")
        (tree / "escape").symlink_to("..", target_is_directory=True)
        os.link(tree / "child/file", tree / "hardlink")
        output, _ = self.run_lab("lab10", ["list", tree])
        self.assertIn(b"child/file", output)
        self.assertIn(b"links=2", output)
        self.assertNotIn(b"escape/", output)
        target = self.file("target", b"old")
        alias = self.path / "old-alias"
        os.link(target, alias)
        contents = self.file("new", b"new\0bytes")
        self.run_lab("lab10", ["replace", target, contents])
        self.assertEqual(target.read_bytes(), contents.read_bytes())
        self.assertEqual(alias.read_bytes(), b"old")
        self.assertEqual(target.stat().st_mode & 0o777, 0o600)

    def test_atomic_replacement_failures(self):
        target = self.file("target", b"old")
        contents = self.file("contents", b"new")
        preload = {"LD_PRELOAD": str(self.build / "io_failures.so")}
        for setting in ({"LAB_FAIL_WRITE": "1"}, {"LAB_FAIL_SYNC": "1"}):
            self.run_lab("lab10", ["replace", target, contents],
                         env={**preload, **setting}, expected=None)
            self.assertEqual(target.read_bytes(), b"old")
            self.assertFalse(list(self.path.glob(".lab10-*")))
        _, errors = self.run_lab("lab10", ["replace", target, contents],
                                 env={**preload, "LAB_FAIL_SYNC": "2"}, expected=None)
        self.assertIn(b"visible", errors)
        self.assertEqual(target.read_bytes(), b"new")

    def test_tcp_framing_binary_and_client(self):
        process, port = self.server("lab11", ["server", "tcp", "127.0.0.1", 0, 3])
        self.echo(port, b"binary\0payload" * 100, fragments=True)
        self.echo(port, b"")
        output, _ = self.run_lab("lab11", ["client", "tcp", "127.0.0.1", port, "hello world"])
        self.assertEqual(output, b"hello world\n")
        self.assertEqual(process.wait(timeout=3), 0, process.stderr.read())

    def test_udp_and_unix_sockets(self):
        process, port = self.server("lab11", ["server", "udp", "127.0.0.1", 0, 2])
        self.run_lab("lab11", ["client", "udp", "127.0.0.1", port, "datagram"])
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as peer:
            peer.settimeout(3)
            peer.sendto(b"", ("127.0.0.1", port))
            self.assertEqual(peer.recv(1), b"")
        self.assertEqual(process.wait(timeout=3), 0)
        path = self.path / "unix.sock"
        local, _ = self.server("lab11", ["server", "unix", path])
        self.run_lab("lab11", ["client", "unix", path, "local"])
        self.assertEqual(local.wait(timeout=3), 0)
        self.assertFalse(path.exists())
        path.write_text("owned by someone else")
        self.run_lab("lab11", ["server", "unix", path], expected=None)
        self.assertEqual(path.read_text(), "owned by someone else")

    def test_socket_rejects_oversized_and_incomplete_frames(self):
        for packet in (struct.pack("!I", 65537), struct.pack("!I", 10) + b"short"):
            process, port = self.server("lab11", ["server", "tcp", "127.0.0.1", 0])
            with socket.create_connection(("127.0.0.1", port), timeout=3) as peer:
                peer.sendall(packet)
                peer.shutdown(socket.SHUT_WR)
            self.assertNotEqual(process.wait(timeout=3), 0)

    def test_epoll_concurrent_fragmented_and_half_closed_sessions(self):
        process, port = self.server("lab12", [0, 40, 1500])
        with concurrent.futures.ThreadPoolExecutor(max_workers=12) as workers:
            list(workers.map(lambda i: self.echo(port, bytes([i]) * (i * 117), i % 2 == 0), range(40)))
        self.assertEqual(process.wait(timeout=4), 0, process.stderr.read())

    def test_epoll_timeouts_shutdown_and_admission_bound(self):
        process, port = self.server("lab12", [0, 0, 150])
        peers = []
        try:
            # More sessions than the fixed client table; later accepts resume.
            for _ in range(70):
                peer = socket.create_connection(("127.0.0.1", port), timeout=3)
                peer.settimeout(3)
                peers.append(peer)
            for peer in peers:
                self.assertEqual(peer.recv(1), b"")
            self.echo(port, b"server still responsive")
            process.send_signal(signal.SIGTERM)
            self.assertEqual(process.wait(timeout=3), 0)
        finally:
            for peer in peers:
                peer.close()
        process, port = self.server("lab12", [0, 0, 200])
        with socket.create_connection(("127.0.0.1", port), timeout=3) as peer:
            peer.sendall(b"\0")
            process.send_signal(signal.SIGTERM)
            self.assertEqual(process.wait(timeout=3), 0)

    def test_epoll_partial_writes_and_malformed_peer_isolation(self):
        process = self.start("lab12", [0, 3, 1500],
                             {"LD_PRELOAD": str(self.build / "io_failures.so"), "LAB_SHORT_SEND": "1"})
        port = int(re.search(rb"port=(\d+)", self.read_until(process.stdout, b"\n"))[1])
        with socket.create_connection(("127.0.0.1", port), timeout=3) as peer:
            peer.sendall(struct.pack("!I", 65537))
            peer.shutdown(socket.SHUT_WR)
            self.assertEqual(peer.recv(1), b"")
        self.echo(port, bytes(range(256)) * 256, fragments=True)
        self.echo(port, b"still serving")
        self.assertEqual(process.wait(timeout=3), 0, process.stderr.read())

    def test_shell_pipelines_redirection_quotes_and_status(self):
        script = (b"printf 'hello world\\n' | tr a-z A-Z > result\n"
                  b"printf 'again\\n' >> result\ncat < result\n"
                  b"printf '%s\\n' \"quoted | token\" escaped\\ word\nexit 0\n")
        output, _ = self.run_lab("lab13", data=script)
        self.assertIn(b"HELLO WORLD\nagain\n", output)
        self.assertIn(b"quoted | token\nescaped word\n", output)
        self.run_lab("lab13", data=b"definitely_missing_program_792\n", expected=127)
        self.run_lab("lab13", data=b"printf x | | cat\n", expected=2)
        self.run_lab("lab13", data=b"sleep 30 &\njobs\nexit 0\n", timeout=3)

    def test_shell_partial_fork_failure_and_large_pipeline(self):
        self.run_lab("lab13", data=b"yes | head -c 200000 | wc -c\nexit 0\n")
        output, _ = self.run_lab("lab13", data=b"printf x | cat\n", expected=1,
                                 env={"LD_PRELOAD": str(self.build / "failures.so"), "LAB_FAIL_FORK": "2"})
        self.assertNotIn(b"x", output)

    def test_shell_terminal_stop_resume_and_interrupt(self):
        pid, terminal = pty.fork()
        if pid == 0:
            os.chdir(self.path)
            os.execl(str(self.build / "lab13"), "lab13")
        reaped = False
        try:
            self.read_until(terminal, b"lab13$ ")
            os.write(terminal, b"sleep 30\n")
            time.sleep(0.08)
            os.write(terminal, b"\x1a")  # Ctrl+Z is delivered to the foreground group.
            self.read_until(terminal, b"lab13$ ")
            os.write(terminal, b"jobs\n")
            self.assertIn(b"stopped", self.read_until(terminal, b"lab13$ "))
            os.write(terminal, b"bg 1\n")
            self.read_until(terminal, b"lab13$ ")
            os.write(terminal, b"fg 1\n")
            time.sleep(0.08)
            os.write(terminal, b"\x03")
            self.read_until(terminal, b"lab13$ ")
            os.write(terminal, b"printf 'terminal alive\\n'\n")
            self.assertIn(b"terminal alive\r\n", self.read_until(terminal, b"lab13$ "))
            os.write(terminal, b"exit 0\n")
            deadline = time.monotonic() + 3
            while time.monotonic() < deadline:
                result, status = os.waitpid(pid, os.WNOHANG)
                if result:
                    reaped = True
                    self.assertEqual(os.waitstatus_to_exitcode(status), 0)
                    break
                time.sleep(0.01)
            self.assertTrue(reaped, "Terminal shell did not exit")
        finally:
            os.close(terminal)  # A terminal hangup also reaches its foreground job.
            if not reaped:
                os.kill(pid, signal.SIGKILL)
                os.waitpid(pid, 0)

    def test_service_reload_rejection_readiness_and_shutdown(self):
        config = self.file("service.conf", "interval_ms=20\nmessage=first\n")
        notify_path = str(self.path / "notify.sock")
        with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as manager:
            manager.bind(notify_path)
            manager.settimeout(3)
            process = self.start("lab14", [config], {"NOTIFY_SOCKET": notify_path})
            self.assertIn(b"READY=1", manager.recv(4096))
            self.read_until(process.stdout, b"message=first")
            updated = self.file("next", "interval_ms=10\nmessage=second\n")
            os.replace(updated, config)
            process.send_signal(signal.SIGHUP)
            self.read_until(process.stdout, b"event=reloaded")
            self.read_until(process.stdout, b"event=tick")
            config.write_text("interval_ms=bad\n")
            process.send_signal(signal.SIGHUP)
            self.read_until(process.stderr, b"event=reload_rejected")
            self.assertIn(b"message=second", self.read_until(process.stdout, b"event=tick"))
            process.send_signal(signal.SIGTERM)
            self.assertEqual(process.wait(timeout=3), 0)
            self.assertIn(b"STOPPING=1", manager.recv(4096))
        self.run_lab("lab14", [config], expected=None)
        config.write_text("interval_ms=1\n")
        output, _ = self.run_lab("lab14", [config, 2])
        self.assertIn(b"event=stopped ticks=2", output)
        abstract = f"lab14-test-{os.getpid()}-{time.monotonic_ns()}"
        with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as manager:
            manager.bind("\0" + abstract)
            manager.settimeout(3)
            self.run_lab("lab14", [config, 1], env={"NOTIFY_SOCKET": "@" + abstract})
            self.assertIn(b"READY=1", manager.recv(4096))
            self.assertIn(b"STOPPING=1", manager.recv(4096))

    def test_credentials_limits_and_seccomp(self):
        self.run_lab("lab15", ["inspect"])
        self.run_lab("lab15", ["cgroup"])
        self.run_lab("lab15", ["cgroup", self.path], expected=77)
        self.run_lab("lab15", ["limits"])
        process = self.start("lab15", ["sandbox"])
        output, errors = process.communicate(timeout=4)
        if process.returncode == 77:
            self.skipTest(errors.decode())
        self.assertEqual(process.returncode, 0, errors)
        self.assertIn(b"getppid=-1", output)
        self.assertIn(b"parent limits_and_hostname_unchanged=1", output)

    def test_namespaces_or_explicit_host_restriction(self):
        process = self.start("lab15", ["namespaces"])
        output, errors = process.communicate(timeout=4)
        if process.returncode == 77:
            self.assertIn(b"SKIP:", errors)
            self.skipTest(errors.decode().strip())
        self.assertEqual(process.returncode, 0, errors)
        self.assertIn(b"namespace_hostname=lab15-isolated", output)

    def test_delegated_cgroup(self):
        parent = os.environ.get("LAB_CGROUP_PARENT")
        if not parent:
            self.skipTest("Set LAB_CGROUP_PARENT to an explicitly delegated cgroup v2 directory")
        process = self.start("lab15", ["cgroup", parent])
        _, errors = process.communicate(timeout=4)
        if process.returncode == 77:
            self.skipTest(errors.decode().strip())
        self.assertEqual(process.returncode, 0, errors)

    def test_diagnostics_descriptor_accounting_and_strace(self):
        source = self.file("source", b"abc\0def")
        output, _ = self.run_lab("lab16", [source, 4])
        match = re.search(rb"fds_before=(\d+) fds_after=(\d+)", output)
        self.assertEqual(match[1], match[2])
        self.assertIn(b"bytes=28", output)
        if not shutil.which("strace"):
            self.skipTest("strace is not installed")
        result = subprocess.run(["strace", "-e", "trace=openat,read,close", "-o", str(self.path / "trace"),
                                 str(self.build / "lab16"), str(source), "2"], capture_output=True, timeout=5)
        if result.returncode and b"Operation not permitted" in result.stderr:
            self.skipTest("Host policy blocks tracing")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(str(source), (self.path / "trace").read_text())

    def test_atomics_totals_publication_and_startup_failure(self):
        output, _ = self.run_lab("lab17", [6, 10000])
        self.assertIn(b"counter=60000 joined_total=60000", output)
        self.run_lab("publication")
        self.run_lab("lab17", [4, 100], expected=1,
                     env={"LD_PRELOAD": str(self.build / "failures.so"), "LAB_FAIL_THREAD": "2"})
        self.run_lab("lab17", [2, 2**64 - 1], expected=None)

    def test_plugin_lifetime_errors_and_abi(self):
        plugin = self.build / "plugin.so"
        output, _ = self.run_lab("lab18", [plugin, 19, 23])
        self.assertIn(b"result=42", output)
        self.run_lab("lab18", [plugin, 2147483647, 1], expected=None)
        self.run_lab("lab18", [self.path / "missing.so", 1, 2], expected=None)
        fixture = self.file("wrong.c", '#include "plugin_api.h"\nconst LabPlugin lab_plugin = {.abi=99};\n')
        self.compile(fixture, self.path / "wrong.so", "-shared", "-fPIC", "-I" + str(ROOT / "labs/lab18"))
        self.run_lab("lab18", [self.path / "wrong.so", 1, 2], expected=None)

    def test_file_mapping_private_shared_and_bounds(self):
        file = self.file("mapped", b"hello world")
        self.run_lab("lab19", [file, "private", 6, "earth"])
        self.assertEqual(file.read_bytes(), b"hello world")
        self.run_lab("lab19", [file, "shared", 6, "earth"])
        self.assertEqual(file.read_bytes(), b"hello earth")
        self.run_lab("lab19", [file, "shared", 10, "overflow"], expected=None)
        self.run_lab("lab19", [self.file("empty", b""), "private", 0, ""], expected=None)

    def test_optional_io_uring_copy(self):
        if not self.uring_available:
            self.skipTest("liburing development files are not available via pkg-config")
        source = self.file("source", bytes(range(256)) * 1000)
        target = self.path / "target"
        process = self.start("lab20", [source, target])
        output, errors = process.communicate(timeout=5)
        if process.returncode == 77:
            self.assertIn(b"SKIP:", errors)
            self.assertFalse(target.exists(), "Restricted io_uring must not truncate a destination")
            self.skipTest(errors.decode().strip())
        self.assertEqual(process.returncode, 0, errors)
        self.assertEqual(target.read_bytes(), source.read_bytes())
        self.run_lab("lab20", [source, source], expected=None)


if __name__ == "__main__":
    unittest.main()
