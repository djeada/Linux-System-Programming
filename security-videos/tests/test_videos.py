"""Smoke tests: safe-to-run examples, always with dummy local data."""
import pathlib
import re
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def program(slug, variant, *args, cwd=None):
    binary = ROOT / "build" / slug / variant
    return subprocess.run([str(binary), *args], cwd=cwd,
                          capture_output=True, text=True, timeout=5)


class SecurityVideoTests(unittest.TestCase):
    def test_admin_checked_input(self):
        slug = "01-admin-flag-overwrite"
        self.assertIn("admin=0", program(slug, "fixed", "hello").stdout)
        self.assertNotEqual(0, program(slug, "fixed", "AAAAAAAAA").returncode)

    def test_format_string_is_literal(self):
        result = program("02-format-string", "fixed", "test %p")
        self.assertEqual(0, result.returncode)
        self.assertEqual("test %p\n", result.stdout)

    def test_shell_injection_is_removed(self):
        slug = "03-command-injection"
        attack = "missing; echo INJECTED; #"
        self.assertIn("INJECTED", program(slug, "vulnerable", attack).stdout)
        self.assertNotIn("INJECTED", program(slug, "fixed", attack).stdout)

    def test_directory_traversal_rejected(self):
        slug = "04-path-traversal"
        with tempfile.TemporaryDirectory() as directory:
            here = pathlib.Path(directory)
            (here / "notes").mkdir()
            (here / "private.txt").write_text("SECRET-DEMO\n")
            (here / "notes" / "hello.txt").write_text("PUBLIC\n")
            self.assertIn("SECRET-DEMO", program(slug, "vulnerable", "../private.txt", cwd=here).stdout)
            self.assertNotEqual(0, program(slug, "fixed", "../private.txt", cwd=here).returncode)
            self.assertIn("PUBLIC", program(slug, "fixed", "hello.txt", cwd=here).stdout)
            (here / "notes" / "link").symlink_to("../private.txt")
            self.assertNotEqual(0, program(slug, "fixed", "link", cwd=here).returncode)

    def test_heap_copy_is_bounded(self):
        slug = "05-heap-overflow"
        self.assertEqual(0, program(slug, "fixed", "hello").returncode)
        self.assertNotEqual(0, program(slug, "fixed", "A" * 30).returncode)

    def test_contact_lifetime(self):
        self.assertIn("Ada", program("06-use-after-free", "fixed", "Ada").stdout)

    def test_double_free_fixed(self):
        self.assertEqual(0, program("07-double-free", "fixed").returncode)

    def test_integer_size(self):
        slug = "08-integer-overflow"
        self.assertEqual(0, program(slug, "fixed", "4").returncode)
        self.assertNotEqual(0, program(slug, "fixed", "4100").returncode)

    def test_signed_size(self):
        slug = "09-negative-length"
        self.assertNotEqual(0, program(slug, "fixed", "-1").returncode)
        self.assertEqual(0, program(slug, "fixed", "7").returncode)
        self.assertNotEqual(0, program(slug, "fixed", "garbage").returncode)

    def test_loop_boundary(self):
        self.assertEqual(0, program("10-off-by-one", "fixed").returncode)

    def test_terminated_string(self):
        slug = "11-unterminated-string"
        self.assertEqual(0, program(slug, "fixed", "seven").returncode)
        self.assertNotEqual(0, program(slug, "fixed", "ABCDEFGH").returncode)

    def test_packet_length(self):
        slug = "12-packet-length"
        self.assertNotEqual(0, program(slug, "fixed", "200").returncode)
        self.assertIn("copied=3", program(slug, "fixed", "3").stdout)

    def test_complete_password(self):
        slug = "13-prefix-auth"
        self.assertIn("ACCESS GRANTED", program(slug, "vulnerable", "sec").stdout)
        self.assertIn("DENIED", program(slug, "fixed", "sec").stdout)
        self.assertIn("DENIED", program(slug, "fixed", "").stdout)
        self.assertIn("ACCESS GRANTED", program(slug, "fixed", "secret42").stdout)

    def test_csprng_token(self):
        slug = "14-predictable-token"
        x = program(slug, "vulnerable", "1234")
        y = program(slug, "vulnerable", "1234")
        self.assertEqual(x.stdout, y.stdout)
        fixed = program(slug, "fixed")
        self.assertEqual(0, fixed.returncode)
        self.assertRegex(fixed.stdout, r"^token=[0-9a-f]{32}\n$")

    def test_tempfile_rejects_symlink(self):
        slug = "15-temp-file-symlink"
        with tempfile.TemporaryDirectory() as directory:
            here = pathlib.Path(directory)
            (here / "scratch").mkdir()
            (here / "victim.txt").write_text("ORIGINAL\n")
            (here / "scratch" / "report.tmp").symlink_to("../victim.txt")
            self.assertNotEqual(0, program(slug, "fixed", cwd=here).returncode)
            self.assertEqual("ORIGINAL\n", (here / "victim.txt").read_text())
            self.assertEqual(0, program(slug, "vulnerable", cwd=here).returncode)
            self.assertEqual("REPORT\n", (here / "victim.txt").read_text())


if __name__ == "__main__":
    unittest.main()
