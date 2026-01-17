#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
import shutil
from pathlib import Path

from defaults import DEFAULT_CLAZY_CHECKS
from utils import (
    run_command,
    init_environment,
    write_file,
    append_file,
    emit_problem_matchers,
    add_common_arguments,
    expand_files,
)


def generate_markdown_report(
    aggregated_output: str, errors_count: int, warnings_count: int, notes_count: int
) -> str:
    """Generate a Markdown report based on clazy output and issue counts."""
    report_lines = []
    if errors_count > 0:
        report_lines.append(
            f"<details><summary>:fire: Clazy found :fire: {errors_count} errors, "
            f":warning: {warnings_count} warnings and :pencil2: {notes_count} notes</summary>"
        )
    elif warnings_count > 0:
        report_lines.append(
            f"<details><summary>:warning: Clazy found :warning: {warnings_count} warnings "
            f"and :pencil2: {notes_count} notes</summary>"
        )
    elif notes_count > 0:
        report_lines.append(
            f"<details><summary>:pencil2: Clazy found :pencil2: {notes_count} notes</summary>"
        )
    else:
        report_lines.append(
            "<details><summary>:heavy_check_mark: Clazy found no errors, warnings or notes</summary>"
        )
    report_lines.append("")
    report_lines.append(
        "[List of checks](https://github.com/KDE/clazy#list-of-checks), "
        "[This explains some of the clazy warnings](https://www.kdab.com/uncovering-32-qt-best-practices-compile-time-clazy/)"
    )
    report_lines.append("````")
    report_lines.append(aggregated_output)
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    return "\n".join(report_lines)


def main():
    parser = argparse.ArgumentParser(
        description="Run Clazy on provided C++ files and append a Markdown report."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--clazy-checks",
        default="",
        help="Comma-separated list of clazy checks to run (default: repo defaults).",
    )
    parser.add_argument(
        "--build-dir",
        default="build",
        help="CMake build directory containing compile_commands.json (default: build).",
    )
    parser.add_argument(
        "--export-fixes",
        default="clazy.yaml",
        help="Path to write clazy fixes YAML (default: clazy.yaml).",
    )
    args = parser.parse_args()
    init_environment(args)

    if shutil.which("clazy-standalone") is None:
        logging.error("clazy-standalone not found in PATH")
        sys.exit(127)

    clazy_checks = (args.clazy_checks or "").strip() or DEFAULT_CLAZY_CHECKS

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

    output = ""
    tool_failed = False
    file_list = expand_files(args.files)
    if not file_list:
        sys.exit(0)

    for file in file_list:
        cmd = [
            "clazy-standalone",
            f"--export-fixes={args.export_fixes}",
            f"-checks={clazy_checks}",
            "-p",
            str(compile_commands),
            file,
        ]
        stdout, stderr, rc = run_command(cmd)
        output += stdout + "\n" + stderr + "\n"
        if rc != 0:
            tool_failed = True

    log_file = os.path.join(args.log_dir, "clazy.log")
    write_file(log_file, output)
    emit_problem_matchers(log_file, "clang.json", "clang")

    error_pattern = r"^.+:\d+:\d+: error: .+$"
    warning_pattern = r"^.+:\d+:\d+: warning: .+$"
    note_pattern = r"^.+:\d+:\d+: note: .+$"
    errors_count = len(re.findall(error_pattern, output, re.MULTILINE))
    warnings_count = len(re.findall(warning_pattern, output, re.MULTILINE))
    notes_count = len(re.findall(note_pattern, output, re.MULTILINE))

    logging.info(
        f"Found {errors_count} errors, {warnings_count} warnings, {notes_count} notes"
    )

    report = generate_markdown_report(output, errors_count, warnings_count, notes_count)
    append_file(args.report_file, report + "\n")

    if tool_failed:
        sys.exit(2)
    sys.exit(0 if errors_count == 0 else 1)


if __name__ == "__main__":
    main()
