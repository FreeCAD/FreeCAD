# SPDX-License-Identifier: LGPL-2.1-or-later

"""Black-box characterization tests for FreeCAD's command-line contract."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class CommandLineTest(unittest.TestCase):
    executable: Path
    gui_executable: Path | None

    def setUp(self):
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.work_directory = Path(self.temporary_directory.name)
        for directory_name in ("freecad-home", "freecad-data", "freecad-temp"):
            (self.work_directory / directory_name).mkdir()

    def run_freecad(self, *arguments: str) -> subprocess.CompletedProcess[str]:
        return self.run_executable(self.executable, *arguments)

    def run_executable(
        self,
        executable: Path,
        *arguments: str,
        environment_overrides: dict[str, str] | None = None,
    ) -> subprocess.CompletedProcess[str]:
        environment = os.environ.copy()
        # Keep tests independent of the invoking user's FreeCAD configuration.
        environment.update(
            {
                "HOME": str(self.work_directory),
                "XDG_CACHE_HOME": str(self.work_directory / "cache"),
                "XDG_CONFIG_HOME": str(self.work_directory / "config"),
                "XDG_DATA_HOME": str(self.work_directory / "data"),
                "FREECAD_USER_HOME": str(self.work_directory / "freecad-home"),
                "FREECAD_USER_DATA": str(self.work_directory / "freecad-data"),
                "FREECAD_USER_TEMP": str(self.work_directory / "freecad-temp"),
            }
        )
        if environment_overrides:
            environment.update(environment_overrides)
        return subprocess.run(
            [str(executable), *arguments],
            cwd=self.work_directory,
            env=environment,
            capture_output=True,
            check=False,
            encoding="utf-8",
            timeout=30,
        )

    def assert_success(self, result: subprocess.CompletedProcess[str]) -> None:
        self.assertEqual(result.returncode, 0, result.stderr)

    def blocked_user_state(self) -> dict[str, str]:
        blocked_parent = self.work_directory / "not-a-directory"
        blocked_parent.write_text("file blocks directory creation", encoding="utf-8")
        return {
            "HOME": str(blocked_parent / "home"),
            "XDG_CACHE_HOME": str(blocked_parent / "cache"),
            "XDG_CONFIG_HOME": str(blocked_parent / "config"),
            "XDG_DATA_HOME": str(blocked_parent / "data"),
            "FREECAD_USER_HOME": str(blocked_parent / "home"),
            "FREECAD_USER_DATA": str(blocked_parent / "data"),
            "FREECAD_USER_TEMP": str(blocked_parent / "temp"),
        }

    def assert_open_files(self, output: str, *filenames: str) -> None:
        self.assertIn(f"OpenFileCount={len(filenames)}\n", output)
        for index, filename in enumerate(filenames):
            self.assertIn(f"OpenFile{index}={filename}\n", output)

    def test_help_is_a_successful_early_exit(self):
        result = self.run_freecad("--help")

        self.assert_success(result)
        self.assertIn("Usage: FreeCAD [options] File1 File2 ...", result.stdout)
        self.assertIn("--response-file", result.stdout)
        self.assertEqual(result.stderr, "")

    def test_help_takes_priority_over_loading_a_response_file(self):
        result = self.run_freecad("@missing-response-file", "--help")

        self.assert_success(result)
        self.assertIn("Usage: FreeCAD [options] File1 File2 ...", result.stdout)
        self.assertNotIn("Could no open the response file", result.stderr)

    def test_basic_version_does_not_require_writable_user_state(self):
        result = self.run_executable(
            self.executable,
            "--version",
            environment_overrides=self.blocked_user_state(),
        )

        self.assert_success(result)
        self.assertRegex(result.stdout, r"^FreeCAD .* Revision: ")
        self.assertEqual(
            (self.work_directory / "not-a-directory").read_text(encoding="utf-8"),
            "file blocks directory creation",
        )

    def test_gui_console_version_does_not_construct_qapplication(self):
        if self.gui_executable is None:
            self.skipTest("GUI executable was not built")
        result = self.run_executable(
            self.gui_executable,
            "--console",
            "--version",
            environment_overrides={
                "DISPLAY": "",
                "WAYLAND_DISPLAY": "",
                "QT_QPA_PLATFORM": "definitely-invalid-platform",
                **self.blocked_user_state(),
            },
        )

        self.assert_success(result)
        self.assertRegex(result.stdout, r"^FreeCAD .* Revision: ")
        self.assertNotIn("platform plugin", result.stderr)
        self.assertEqual(
            (self.work_directory / "not-a-directory").read_text(encoding="utf-8"),
            "file blocks directory creation",
        )

    def test_gui_informational_options_do_not_require_a_display(self):
        if self.gui_executable is None:
            self.skipTest("GUI executable was not built")
        for arguments, expected in (
            (("--help",), "Usage: FreeCAD"),
            (("--version",), "FreeCAD "),
            (("--dump-config",), "ExeName=FreeCAD"),
            (("--get-config", "ExeName"), "FreeCAD\n"),
            (("--verbose", "--version"), "OS:"),
        ):
            with self.subTest(arguments=arguments):
                result = self.run_executable(
                    self.gui_executable,
                    *arguments,
                    environment_overrides={
                        "DISPLAY": "",
                        "WAYLAND_DISPLAY": "",
                        "QT_QPA_PLATFORM": "definitely-invalid-platform",
                    },
                )
                self.assert_success(result)
                self.assertIn(expected, result.stdout)
                self.assertNotIn("platform plugin", result.stderr)

    def test_unknown_option_reports_help_and_fails(self):
        result = self.run_freecad("--definitely-invalid")

        self.assertEqual(result.returncode, 1)
        self.assertIn("--definitely-invalid", result.stderr)
        self.assertIn("Allowed options:", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_gui_unknown_option_reports_to_stderr_without_a_display(self):
        if self.gui_executable is None:
            self.skipTest("GUI executable was not built")
        result = self.run_executable(
            self.gui_executable,
            "--asd",
            environment_overrides={
                "DISPLAY": "",
                "WAYLAND_DISPLAY": "",
                "QT_QPA_PLATFORM": "definitely-invalid-platform",
            },
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("unrecognised option '--asd'", result.stderr)
        self.assertIn("Allowed options:", result.stderr)
        self.assertNotIn("platform plugin", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_command_line_and_config_file_composing_options_are_combined(self):
        (self.work_directory / "FreeCAD.cfg").write_text(
            "module-path=/config/module\n"
            "macro-path=/config/macro\n",
            encoding="utf-8",
        )

        result = self.run_freecad(
            "--module-path",
            "/cli/module",
            "--macro-path",
            "/cli/macro",
            "--dump-config",
        )

        self.assert_success(result)
        self.assertIn("AdditionalModulePaths=/cli/module;/config/module\n", result.stdout)
        self.assertIn("AdditionalMacroPaths=/cli/macro;/config/macro\n", result.stdout)

    def test_command_line_scalar_takes_precedence_over_config_file(self):
        (self.work_directory / "FreeCAD.cfg").write_text(
            "log-file=/config/log\n", encoding="utf-8"
        )

        result = self.run_freecad("--log-file", "/cli/log", "--dump-config")

        self.assert_success(result)
        self.assertIn("LoggingFileName=/cli/log\n", result.stdout)
        self.assertNotIn("LoggingFileName=/config/log\n", result.stdout)

    def test_get_config_is_processed_before_set_config(self):
        result = self.run_freecad(
            "--set-config", "ExeName=Changed", "--get-config", "ExeName"
        )

        self.assert_success(result)
        self.assertEqual(result.stdout, "FreeCAD\n")

    def test_test_selection_rules(self):
        cases = (
            (
                ("--run-test", "First.Suite", "-t", "Second.Suite"),
                ("TestCase=First.Suite,Second.Suite", "ExitTests=yes"),
            ),
            (("-t", "-t", "Named.Suite"), ("TestCase=TestApp.PrintAll",)),
            (
                ("--run-open", "0", "--run-test", "Named.Suite"),
                ("TestCase=TestApp.All", "ExitTests=no"),
            ),
        )
        for arguments, expected_lines in cases:
            with self.subTest(arguments=arguments):
                result = self.run_freecad(*arguments, "--dump-config")
                self.assert_success(result)
                for line in expected_lines:
                    self.assertIn(f"{line}\n", result.stdout)

    def test_missing_response_file_is_a_command_line_error(self):
        result = self.run_freecad("@missing-response-file")

        self.assertEqual(result.returncode, 1)
        self.assertIn(
            "Could no open the response file: 'missing-response-file'", result.stderr
        )

    def test_response_file_can_supply_flag_options(self):
        (self.work_directory / "arguments.rsp").write_text(
            "--dump-config\n", encoding="utf-8"
        )

        result = self.run_freecad("@arguments.rsp")

        self.assert_success(result)
        self.assertIn("ExeName=FreeCAD\n", result.stdout)

    def test_response_file_options_compose_and_preserve_order(self):
        (self.work_directory / "arguments.rsp").write_text(
            "--module-path /response/one "
            "--module-path /response/two "
            "first.FCStd second.py\n",
            encoding="utf-8",
        )

        result = self.run_freecad(
            "--module-path", "/cli/module", "@arguments.rsp", "--dump-config"
        )

        self.assert_success(result)
        self.assertIn(
            "AdditionalModulePaths=/cli/module;/response/one;/response/two\n",
            result.stdout,
        )
        self.assert_open_files(result.stdout, "first.FCStd", "second.py")

    def test_positional_files_are_forwarded_in_order(self):
        result = self.run_freecad("first.FCStd", "second.py", "--dump-config")

        self.assert_success(result)
        self.assert_open_files(result.stdout, "first.FCStd", "second.py")

    def test_pass_arguments_remain_visible_to_python(self):
        probe = self.work_directory / "argv_probe.py"
        probe.write_text(
            'import json, sys\nprint("ARGV_JSON=" + json.dumps(sys.argv))\n',
            encoding="utf-8",
        )

        result = self.run_freecad(probe.name, "--pass", "alpha", "two words")

        self.assert_success(result)
        line = next(
            line for line in result.stdout.splitlines() if line.startswith("ARGV_JSON=")
        )
        arguments = json.loads(line.removeprefix("ARGV_JSON="))
        self.assertEqual(arguments[1:], [probe.name, "--pass", "alpha", "two words"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--freecadcmd", required=True, type=Path)
    parser.add_argument("--freecad", type=Path)
    arguments, unittest_arguments = parser.parse_known_args()
    CommandLineTest.executable = arguments.freecadcmd.resolve()
    CommandLineTest.gui_executable = arguments.freecad.resolve() if arguments.freecad else None
    unittest.main(argv=[__file__, *unittest_arguments])
