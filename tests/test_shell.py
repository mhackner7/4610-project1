"""Black-box Linux regression tests; fixtures live outside the source tree."""
import os
from pathlib import Path
import re
import resource
import subprocess
import sys
import tempfile
import time
import unittest

SHELL = str(Path(sys.argv.pop(1)).resolve())


class ShellTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="cop4610-tests-")
        self.cwd = Path(self.temp.name)
        self.env = dict(os.environ, PATH="/usr/bin:/bin", USER="testuser",
                        HOME=str(self.cwd), TEST_VALUE="two words",
                        TEST_TILDE="~")
        self.env.pop("COP4610_UNSET", None)

    def tearDown(self):
        self.temp.cleanup()

    def run_shell(self, commands, env=None, limit_fds=False):
        def restrict():
            resource.setrlimit(resource.RLIMIT_NOFILE, (32, 32))
        result = subprocess.run(
            [SHELL], input=commands, text=True, capture_output=True,
            cwd=self.cwd, env=self.env if env is None else env, timeout=12,
            preexec_fn=restrict if limit_fds else None)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("AddressSanitizer", result.stderr)
        self.assertNotIn("runtime error:", result.stderr)
        return result

    def output(self, result):
        return re.sub(r"testuser@[^\n>]*>", "", result.stdout).strip()

    def program(self, name, body, directory=None):
        path = (directory or self.cwd) / name
        path.write_text("#!/bin/sh\n" + body + "\n")
        path.chmod(0o700)
        return path

    def test_prompt_and_blank_eof(self):
        result = self.run_shell("\n\t\n")
        self.assertIn(f"testuser@{os.uname().nodename}:{self.cwd}>", result.stdout)
        self.assertEqual(self.output(result), "")
        self.assertEqual(result.stderr, "")

    def test_final_line_without_newline(self):
        for length in range(1, 10):
            self.assertEqual(self.output(self.run_shell("echo " + "x" * length)),
                             "x" * length)

    def test_expansions(self):
        result = self.run_shell(
            "echo $TEST_VALUE\necho $COP4610_UNSET\necho ~ ~/dir ~other\n"
            "echo $TEST_TILDE\necho $PATH\necho $PATH\n")
        self.assertEqual(self.output(result).splitlines(), [
            "two words", "", f"{self.cwd} {self.cwd}/dir ~other", "~",
            "/usr/bin:/bin", "/usr/bin:/bin"])

    def test_expansion_is_one_argument(self):
        self.program("argc", 'printf "%s\\n" "$#" "$1"')
        self.assertEqual(self.output(self.run_shell("./argc $TEST_VALUE\n")),
                         "1\ntwo words")

    def test_path_order_and_skip_nonexecutables(self):
        first, second = self.cwd / "first", self.cwd / "second"
        first.mkdir()
        second.mkdir()
        a = self.program("pick", "echo first", first)
        self.program("pick", "echo second", second)
        env = dict(self.env, PATH=f"{first}:{second}")
        self.assertEqual(self.output(self.run_shell("pick\n", env)), "first")
        a.chmod(0o600)
        self.assertEqual(self.output(self.run_shell("pick\n", env)), "second")

    def test_empty_and_unset_path(self):
        self.program("local", "echo local")
        for path in ("", ":/nonexistent", "/nonexistent:", "/x::/y"):
            result = self.run_shell("local\n", dict(self.env, PATH=path))
            self.assertEqual(self.output(result), "local")
        env = self.env.copy()
        env.pop("PATH")
        result = self.run_shell("local\n/bin/echo explicit\n", env)
        self.assertIn("command not found", result.stderr)
        self.assertEqual(self.output(result), "explicit")

    def test_paths_arguments_and_recovery(self):
        self.program("local", 'printf "%s\\n" "$1" "$2"')
        result = self.run_shell(
            f"./local one two\n{self.cwd}/local three four\n"
            "not_a_real_command_4610\n/bin/echo alive\n")
        self.assertEqual(self.output(result), "one\ntwo\nthree\nfour\nalive")
        self.assertIn("command not found", result.stderr)

    def test_nonexecutable_directory_and_exec_failure(self):
        (self.cwd / "plain").write_text("no executable bit")
        broken = self.cwd / "broken"
        broken.write_text("not a valid executable")
        broken.chmod(0o700)
        result = self.run_shell("./plain\n./\n./broken\necho survived\n")
        self.assertEqual(self.output(result), "survived")
        self.assertIn("Permission denied", result.stderr)
        self.assertIn("Exec format error", result.stderr)

    def test_wait_and_nonzero_exit(self):
        start = time.monotonic()
        result = self.run_shell("sleep 0.2\nfalse\necho done\n")
        self.assertGreaterEqual(time.monotonic() - start, 0.18)
        self.assertEqual(self.output(result), "done")

    def test_output_permissions_truncation_and_parent_streams(self):
        output = self.cwd / "out"
        result = self.run_shell("echo long-first-output > out\necho hi > out\necho screen\n")
        self.assertEqual(output.read_text(), "hi\n")
        self.assertEqual(output.stat().st_mode & 0o777, 0o600)
        self.assertEqual(self.output(result), "screen")
        output.chmod(0o666)
        self.run_shell("echo again > out\n")
        self.assertEqual(output.stat().st_mode & 0o777, 0o600)

    def test_input_and_both_orders(self):
        source = self.cwd / "in"
        source.write_text("z\na\nb\n")
        result = self.run_shell("sort < in\nsort < in > out1\nsort > out2 < in\n")
        self.assertEqual(self.output(result), "a\nb\nz")
        for name in ("out1", "out2"):
            self.assertEqual((self.cwd / name).read_text(), "a\nb\nz\n")
        self.assertEqual(source.read_text(), "z\na\nb\n")

    def test_expanded_redirection_paths(self):
        (self.cwd / "in").write_text("hello\n")
        env = dict(self.env, DEST=str(self.cwd / "output with spaces"))
        self.run_shell("cat < ~/in > $DEST\n", env)
        self.assertEqual((self.cwd / "output with spaces").read_text(), "hello\n")

    def test_bad_input_does_not_truncate_output(self):
        target = self.cwd / "out"
        target.write_text("keep")
        os.mkfifo(self.cwd / "fifo")
        for source in ("missing", ".", "fifo"):
            result = self.run_shell(f"cat > out < {source}\necho alive\n")
            self.assertTrue(result.stderr)
            self.assertEqual(target.read_text(), "keep")
            self.assertEqual(self.output(result), "alive")

    def test_same_file_hardlink_and_symlink(self):
        source = self.cwd / "in"
        source.write_text("keep input")
        os.link(source, self.cwd / "hard")
        (self.cwd / "sym").symlink_to(source)
        for target in ("in", "hard", "sym"):
            result = self.run_shell(f"cat < in > {target}\n")
            self.assertIn("same file", result.stderr)
            self.assertEqual(source.read_text(), "keep input")

    def test_malformed_redirection(self):
        for command in ("echo >", "cat <", "> out", "echo > > out",
                        "echo > one > two", "cat < one < two", "echo > $COP4610_UNSET"):
            result = self.run_shell(command + "\necho recovered\n")
            self.assertTrue(result.stderr, command)
            self.assertEqual(self.output(result), "recovered")
        self.assertFalse((self.cwd / "out").exists())

    @unittest.skipIf(os.geteuid() == 0, "root bypasses ordinary permission checks")
    def test_permission_failures(self):
        source = self.cwd / "unreadable"
        source.write_text("secret")
        source.chmod(0)
        out = self.cwd / "readonly"
        out.write_text("keep")
        out.chmod(0o400)
        result = self.run_shell("cat < unreadable\necho fail > readonly\necho alive\n")
        self.assertEqual(result.stderr.count("Permission denied"), 2)
        self.assertEqual(out.read_text(), "keep")
        self.assertEqual(self.output(result), "alive")

    def test_bad_output_and_stderr_not_redirected(self):
        result = self.run_shell("echo x > missing/out\necho x > .\nls missing > out\n")
        self.assertTrue(result.stderr)
        self.assertEqual((self.cwd / "out").read_text(), "")

    def test_reserved_team_features(self):
        result = self.run_shell("echo no | cat\necho no &\ncd /\njobs\nexit\necho yes\n")
        self.assertEqual(self.output(result), "yes")
        self.assertIn("parts 7-8", result.stderr)
        self.assertIn("part 9", result.stderr)

    def test_repeated_commands_with_low_descriptor_limit(self):
        (self.cwd / "in").write_text("data\n")
        commands = "cat < in > out\n" * 100 + "echo finished\n"
        result = self.run_shell(commands, limit_fds=True)
        self.assertEqual(result.stderr, "")
        self.assertEqual(self.output(result), "finished")


if __name__ == "__main__":
    unittest.main(verbosity=2)
