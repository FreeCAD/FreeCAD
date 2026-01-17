#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
import shutil
from pathlib import Path

from defaults import DEFAULT_CLAZY_QT6_CHECKS
from utils import (
    run_command,
    init_environment,
    write_file,
    append_file,
    emit_problem_matchers,
    add_common_arguments,
    expand_files,
)


def count_occurrences(pattern, text):
    matches = re.findall(pattern, text, re.MULTILINE)
    logging.debug(f"Found {len(matches)} matches for pattern: {pattern}")
    return len(matches)


def generate_markdown_report(
    output: str, errors_count: int, warnings_count: int, notes_count: int
) -> str:
    """Generate a Markdown report based on Clazy QT6 output and issue counts."""
    report_lines = []
    if errors_count > 0:
        report_lines.append(
            f"<details><summary>:fire: Clazy found :fire: {errors_count} errors, "
            f":warning: {warnings_count} warnings and :pencil2: {notes_count} notes for porting to QT6</summary>"
        )
    elif warnings_count > 0:
        report_lines.append(
            f"<details><summary>:warning: Clazy found :warning: {warnings_count} warnings and "
            f":pencil2: {notes_count} notes for porting to QT6</summary>"
        )
    elif notes_count > 0:
        report_lines.append(
            f"<details><summary>:pencil2: Clazy found :pencil2: {notes_count} notes for porting to QT6</summary>"
        )
    else:
        report_lines.append(
            "<details><summary>:heavy_check_mark: Clazy found no errors, warnings or notes for porting to QT6</summary>"
        )
    report_lines.append("")
    report_lines.append("````")
    report_lines.append(output)
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    return "\n".join(report_lines)


def main():
    parser = argparse.ArgumentParser(
        description="Run Clazy checks for QT6 on provided C++ files and append a Markdown report."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--clazy-qt6-checks",
        default="",
        help="Comma-separated list of clazy QT6 checks to run (default: repo defaults).",
    )
    parser.add_argument(
        "--build-dir",
        default="build",
        help="CMake build directory containing compile_commands.json (default: build).",
    )
    parser.add_argument(
        "--export-fixes",
        default="clazyQT6.yaml",
        help="Path to write clazy QT6 fixes YAML (default: clazyQT6.yaml).",
    )
    args = parser.parse_args()

    # Initialize logging and required directories using a shared utility.
    init_environment(args)

    if shutil.which("clazy-standalone") is None:
        logging.error("clazy-standalone not found in PATH")
        sys.exit(127)

    clazy_checks = (args.clazy_qt6_checks or "").strip() or DEFAULT_CLAZY_QT6_CHECKS

    build_dir = Path(args.build_dir)
    if not build_dir.is_absolute():
        build_dir = (Path.cwd() / build_dir).resolve()
    if not build_dir.is_dir():
        logging.error("build dir does not exist: %s", build_dir)
        sys.exit(2)
    compile_commands = build_dir / "compile_commands.json"
    if not compile_commands.is_file():
        logging.error("missing compile_commands.json: %s", compile_commands)
        logging.error("Configure your build with CMAKE_EXPORT_COMPILE_COMMANDS=ON")
        sys.exit(2)

    files = expand_files(args.files)
    if not files:
        sys.exit(0)

    # Build the clazy command as a list.
    cmd = [
        "clazy-standalone",
        f"--export-fixes={args.export_fixes}",
        f"-checks={clazy_checks}",
        "-p",
        str(compile_commands),
    ] + files

    stdout, stderr, rc = run_command(cmd)
    output = stdout + stderr

    log_file = os.path.join(args.log_dir, "clazyQT6.log")
    write_file(log_file, output)
    emit_problem_matchers(log_file, "clang.json", "clang")
    if rc != 0:
        logging.error("clazy-standalone failed (exit=%s)", rc)
        sys.exit(rc)

    error_pattern = r"^.+:\d+:\d+: error: .+$"
    warning_pattern = r"^.+:\d+:\d+: warning: .+$"
    note_pattern = r"^.+:\d+:\d+: note: .+$"
    errors_count = count_occurrences(error_pattern, output)
    warnings_count = count_occurrences(warning_pattern, output)
    notes_count = count_occurrences(note_pattern, output)

    logging.info(
        f"Found {errors_count} errors, {warnings_count} warnings, {notes_count} notes"
    )

    report = generate_markdown_report(output, errors_count, warnings_count, notes_count)
    append_file(args.report_file, report + "\n")

    sys.exit(0 if errors_count == 0 else 1)


if __name__ == "__main__":
    main()
