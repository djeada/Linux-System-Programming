"""Build every lab and check behavior, shutdown, and partial-startup failures.

Run: python3 -m unittest discover -s tests -v
"""

import collections
import concurrent.futures
import os
from pathlib import Path
import re
import selectors
import shlex
import signal
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]


class Labs(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="linux-labs-tests-")
        cls.addClassCleanup(cls.directory.cleanup)
        cls.build = Path(cls.directory.name)
        cls.compiler = shlex.split(os.environ.get("CC", "cc"))
        cls.flags = shlex.split(os.environ.get(
            "LAB_TEST_CFLAGS", "-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror"
        ))
        for number in range(1, 9):
            subprocess.run(
                [*cls.compiler, *cls.flags, str(ROOT / f"labs/lab{number}/main.c"),
                 "-pthread", "-o", str(cls.build / f"lab{number}")],
                check=True, capture_output=True, text=True,
            )
        subprocess.run(
            [*cls.compiler, *cls.flags, str(ROOT / "labs/lab3/comparisons/fork_exec.c"),
             "-pthread", "-o", str(cls.build / "fork_exec")],
            check=True, capture_output=True, text=True,
        )
        cls.injection = cls.build / "failures.so"
        subprocess.run(
            [*cls.compiler, "-std=c11", "-O2", "-shared", "-fPIC",
             str(ROOT / "tests/fixtures/failures.c"), "-o", str(cls.injection), "-ldl", "-pthread"],
            check=True, capture_output=True, text=True,
        )

    def setUp(self):
        self.work = tempfile.TemporaryDirectory(prefix="lab-case-")
        self.addCleanup(self.work.cleanup)
        self.path = Path(self.work.name)

    def file(self, name, data):
        path = self.path / name
        path.write_bytes(data.encode() if isinstance(data, str) else data)
        return path

    def run_lab(self, lab, args=(), input_data=b"", success=True, env=None, timeout=8):
        settings = {**os.environ, "LAB_DELAY_MS": "0", **(env or {})}
        if settings.get("LAB_DELAY_MS") is None:
            del settings["LAB_DELAY_MS"]
        process = subprocess.Popen(
            [str(self.build / str(lab)), *map(str, args)],
            cwd=self.path, env=settings, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True,
        )
        try:
            output, errors = process.communicate(input_data, timeout=timeout)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
            self.fail(f"{lab} hung, possibly leaving a child or waiter alive")
        if success:
            self.assertEqual(process.returncode, 0, errors.decode(errors="replace"))
        else:
            self.assertNotEqual(process.returncode, 0, output.decode(errors="replace"))
        return output, errors

    @staticmethod
    def read_until(process, needle, timeout):
        output = b""
        deadline = time.monotonic() + timeout
        with selectors.DefaultSelector() as ready:
            ready.register(process.stdout, selectors.EVENT_READ)
            while needle not in output:
                remaining = deadline - time.monotonic()
                if remaining <= 0 or not ready.select(remaining):
                    raise AssertionError(f"No {needle!r} in output: {output!r}")
                chunk = os.read(process.stdout.fileno(), 8192)
                if not chunk:
                    raise AssertionError(f"Process exited before {needle!r}: {output!r}")
                output += chunk
        return output

    def test_maze_commands_eof_and_validation(self):
        maze = self.file("maze", "#####\n#S E#\n#####\n")
        output, _ = self.run_lab("lab1", [maze], b"d\nd\n")
        self.assertIn(b"Congratulations!", output)
        self.run_lab("lab1", [maze])
        for data in ("SS E\n", "S#\nE\n", "S" + " " * 100 + "E\n", "###\n###\n"):
            with self.subTest(data=data[:15]):
                self.run_lab("lab1", [self.file("invalid", data)], success=False)
        full = self.file("max_rows", "S E\n" + "###\n" * 99)
        self.run_lab("lab1", [full])

    def test_maze_signal_delivery_and_idle_timeout(self):
        maze = self.file("maze", "#######\n#S   E#\n#######\n")
        process = subprocess.Popen(
            [str(self.build / "lab1"), str(maze)], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, start_new_session=True,
        )
        try:
            self.read_until(process, b"to move:", 2)
            os.kill(process.pid, signal.SIGTSTP)
            self.read_until(process, b"Cheat", 2)
            self.read_until(process, b"Time's up!", 7)
            # Partial input must not trap the program inside buffered fgets().
            process.stdin.write(b" ")
            process.stdin.flush()
            os.kill(process.pid, signal.SIGINT)
            output, errors = process.communicate(timeout=2)
            self.assertEqual(process.returncode, 0, errors)
            self.assertIn(b"Goodbye", output)
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGKILL)
            process.communicate()

    def test_forked_reader_handles_full_lines_and_child_errors(self):
        source = self.file("lines", "x" * 5000 + "\nlast line")
        output, _ = self.run_lab("lab2", [source])
        self.assertEqual(output.count(b"x"), 10000)
        self.assertEqual(output.count(b"last line"), 2)
        self.assertEqual(len(re.findall(rb"\[PID \d+\]", output)), 4)
        self.assertEqual(len(set(re.findall(rb"\[PID (\d+)\]", output))), 2)
        self.run_lab("lab2", [self.path / "missing"], success=False)

    def test_spawn_shell_and_fork_comparison_keep_whole_commands(self):
        commands = b"  \n/nonexistent/lab-command\necho " + b"z" * 5000 + b"\nexit\n"
        for lab in ("lab3", "fork_exec"):
            with self.subTest(lab=lab):
                output, errors = self.run_lab(lab, input_data=commands)
                self.assertEqual(output.count(b"z"), 5000)
                self.assertTrue(errors)
                self.run_lab(lab, input_data=b"echo ignored\x00 hidden\nexit\n")

    def test_barber_tracks_each_completed_haircut(self):
        for chairs, customers in ((0, 6), (3, 12), (2, 0)):
            with self.subTest(chairs=chairs, customers=customers):
                output, _ = self.run_lab("lab4", [chairs, customers], env={"LAB_DELAY_MS": "2"})
                completed = re.findall(rb"Barber: Finished haircut for customer (\d+)\.", output)
                paid = re.findall(rb"Customer (\d+): Haircut done", output)
                self.assertEqual(collections.Counter(completed), collections.Counter(paid))
                self.assertEqual(len(paid) + output.count(b"No available chairs"), customers)
                for customer in paid:
                    self.assertLess(output.index(b"Finished haircut for customer " + customer + b"."),
                                    output.index(b"Customer " + customer + b": Haircut done"))

    def test_pipeline_preserves_stream_and_ring_wraparound(self):
        for data, width in ((b"*aB*X***\ntail*", 3), (b"", 20),
                            (b"\x00**\xff\n", 2), ((b"a*b**c\n" * 3000), 71)):
            with self.subTest(length=len(data), width=width):
                source = self.file("input", data)
                output, _ = self.run_lab("lab5", [source, width])
                stream = data.replace(b"\n", b"<EOL>").replace(b"**", b"#")
                expected = b"".join(stream[index:index + width] + b"\n" for index in range(0, len(stream), width))
                self.assertEqual(output, expected)
        self.run_lab("lab5", [self.path / "missing"], success=False)
        self.run_lab("lab5", [self.path], success=False)

    def test_garage_capacity_and_type_exclusion_under_contention(self):
        vehicles = self.file("vehicles", "".join(f"{'C' if index % 2 else 'T'} v{index}\n" for index in range(40)))
        for spots in (1, 3):
            with self.subTest(spots=spots):
                output, _ = self.run_lab("lab6", [spots, 40, vehicles], env={"LAB_DELAY_MS": "2"})
                active = {}
                admitted = set()
                for line in output.decode().splitlines():
                    event = re.match(r"Vehicle (v\d+) \(([CT])\) (parks|leaves)\.", line)
                    if not event:
                        continue
                    name, kind, action = event.groups()
                    if action == "parks":
                        self.assertNotIn(name, admitted)
                        self.assertTrue(all(value == kind for value in active.values()), line)
                        active[name] = kind
                        admitted.add(name)
                        self.assertLessEqual(len(active), spots)
                    else:
                        self.assertIn(name, active)
                        del active[name]
                self.assertFalse(active)
                self.assertEqual(len(admitted), 40)
                for used, capacity in re.findall(rb"Occupancy: (\d+)/(\d+)", output):
                    self.assertLessEqual(int(used), int(capacity))

    def test_bounded_thread_pool_delivers_each_task_once(self):
        data = "".join(f"{index},task {index},0\n" for index in range(600))
        data += "601,,0\n602,bad,-1\n603,extra,0,ignored\n604," + "x" * 700 + ",0\n"
        source = self.file("tasks", data)
        output, errors = self.run_lab("lab7", [4, source])
        completed = re.findall(rb"Completed Task (\d+)", output)
        self.assertEqual(sorted(map(int, completed)), list(range(600)))
        self.assertEqual(errors.count(b"Skipping malformed task"), 4)
        self.run_lab("lab7", [2, self.file("empty", "")])

    def test_restaurant_drains_accepted_orders_and_reports_after_chefs(self):
        source = self.file("orders", "".join(f"{index},1,order {index},0\n" for index in range(200)))
        output, _ = self.run_lab("lab8", [4, 3, source, 2])
        completed = re.findall(rb"Completed Order (\d+)\.", output)
        self.assertEqual(sorted(map(int, completed)), list(range(200)))
        self.assertIn(b"Total orders processed: 200", output)
        self.assertGreater(output.index(b"Final status reported"), output.rindex(b"Completed Order"))
        slow = self.file("slow", "1,1,slow order,1\n")
        output, _ = self.run_lab("lab8", [1, 1, slow, 2], env={"LAB_DELAY_MS": None})
        self.assertGreater(output.index(b"Final status reported"), output.index(b"Completed Order 1."))

    def test_restaurant_deadline_releases_producers_waiting_for_capacity(self):
        source = self.file("many_orders", "".join(f"{index},1,order,0\n" for index in range(10000)))
        output, _ = self.run_lab("lab8", [16, 1, source, 1], env={"LAB_DELAY_MS": "10"}, timeout=5)
        accepted = re.findall(rb"Enqueued Order (\d+) for", output)
        completed = re.findall(rb"Completed Order (\d+)\.", output)
        self.assertLess(len(accepted), 10000)
        self.assertEqual(collections.Counter(accepted), collections.Counter(completed))

    def test_partial_process_startup_and_worker_failure_reap_peers(self):
        source = self.file("pipeline", "x" * 20000)
        vehicles = self.file("vehicles", "C a\nT b\nC c\n")
        cases = (("lab4", [2, 5], 2), ("lab5", [source], 3), ("lab6", [1, 3, vehicles], 3))
        for lab, arguments, call in cases:
            for failure in ("LAB_FAIL_FORK", "LAB_FAIL_CHILD"):
                with self.subTest(lab=lab, failure=failure):
                    self.run_lab(lab, arguments, success=False,
                                 env={"LD_PRELOAD": str(self.injection), failure: str(call)})

    def test_partial_semaphore_and_thread_initialization_cleanup(self):
        source = self.file("pipeline", "text")
        vehicles = self.file("vehicles", "C a\nT b\n")
        for lab, args in (("lab4", [2, 3]), ("lab5", [source]), ("lab6", [1, 2, vehicles])):
            with self.subTest(lab=lab):
                self.run_lab(lab, args, success=False,
                             env={"LD_PRELOAD": str(self.injection), "LAB_FAIL_SEMAPHORE": "2"})
        tasks = self.file("tasks", "1,task,0\n")
        orders = self.file("orders", "1,1,order,0\n")
        for lab, args, call in (("lab7", [4, tasks], 3), ("lab8", [2, 2, orders, 1], 3), ("lab8", [2, 1, orders, 1], 4)):
            with self.subTest(lab=lab, call=call):
                _, errors = self.run_lab(lab, args, success=False,
                                        env={"LD_PRELOAD": str(self.injection), "LAB_FAIL_THREAD": str(call)})
                self.assertNotIn(b"Success", errors)
        self.run_lab("lab7", [2, tasks], success=False,
                     env={"LD_PRELOAD": str(self.injection), "LAB_FAIL_CONDITION": "2"})


if __name__ == "__main__":
    unittest.main()
