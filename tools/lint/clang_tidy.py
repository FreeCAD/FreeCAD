#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
import shutil
from pathlib import Path
from utils import (
    run_command,
    init_environment,
    add_common_arguments,
    emit_problem_matchers,
    write_file,
    append_file,
    expand_files,
)


def generate_markdown_report(
    errors_count: int,
    warnings_count: int,
    notes_count: int,
    clang_tidy_output: str,
    enabled_checks_content: str,
) -> str:
    """Generate a Markdown report section for clang-tidy results and enabled checks."""
    report_lines = []
    if errors_count > 0:
        report_lines.append(
            f"<details><summary>:fire: Clang-Tidy found :fire: {errors_count} errors, "
            f":warning: {warnings_count} warnings and :pencil2: {notes_count} notes</summary>"
        )
    elif warnings_count > 0:
        report_lines.append(
            f"<details><summary>:warning: Clang-Tidy found :warning: {warnings_count} "
            f"warnings and :pencil2: {notes_count} notes</summary>"
        )
    elif notes_count > 0:
        report_lines.append(
            f"<details><summary>:pencil2: Clang-Tidy found :pencil2: {notes_count} notes</summary>"
        )
    else:
        report_lines.append(
            "<details><summary>:heavy_check_mark: Clang-Tidy found no errors, warnings or notes</summary>"
        )
    report_lines.append("")
    report_lines.append("````")
    report_lines.append(clang_tidy_output)
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    report_lines.append(
        "<details><summary>:information_source: Enabled checks</summary>"
    )
    report_lines.append("")
    report_lines.append("````")
    report_lines.append(enabled_checks_content)
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    return "\n".join(report_lines)


def count_occurrences(pattern: str, text: str) -> int:
    """Count all occurrences of a regex pattern in text using MULTILINE mode."""
    matches = re.findall(pattern, text, re.MULTILINE)
    logging.info("Pattern '%s' found %d matches", pattern, len(matches))
    return len(matches)


def main():
    parser = argparse.ArgumentParser(
        description="Run clang-tidy on provided C++ files and append a Markdown report."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--clang-style",
        default="file",
        help="Clang-format style (e.g., 'file' to use .clang-format or a specific style).",
    )
    parser.add_argument(
        "--line-filter",
        required=False,
        help='Line-filter for clang-tidy (i.e. [{"name":"file1.cpp","lines":[[1,3],[5,7]]},...])',
    )
    parser.add_argument(
        "--build-dir",
        default="build",
        help="CMake build directory containing compile_commands.json (default: build).",
    )
    parser.add_argument(
        "--export-fixes",
        default="clang-tidy.yaml",
        help="Path to write clang-tidy fixes YAML (default: clang-tidy.yaml).",
    )
    args = parser.parse_args()
    init_environment(args)

    args.clang_style = (args.clang_style or "file").strip() or "file"

    if shutil.which("clang-tidy") is None:
        logging.error("clang-tidy not found in PATH")
        sys.exit(127)

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

    clang_tidy_base_cmd = [
        "clang-tidy",
        "--quiet",
        f"--format-style={args.clang_style}",
        "-p",
        str(build_dir),
    ]

    enabled_cmd = clang_tidy_base_cmd + ["--explain-config"]
    logging.debug("clang-tidy enabled-checks cmd: %s", " ".join(enabled_cmd))
    enabled_stdout, enabled_stderr, enabled_rc = run_command(enabled_cmd)
    enabled_output = enabled_stdout + enabled_stderr
    enabled_checks_log = os.path.join(args.log_dir, "clang-tidy-enabled-checks.log")
    write_file(enabled_checks_log, enabled_output)
    if enabled_rc != 0:
        logging.error("clang-tidy failed to explain config (exit=%s)", enabled_rc)
        sys.exit(enabled_rc)

    clang_cmd = clang_tidy_base_cmd + [f"--export-fixes={args.export_fixes}"]
    if args.line_filter:
        clang_cmd = clang_cmd + [f"--line-filter={args.line_filter}"]
    files = expand_files(args.files)
    if not files:
        sys.exit(0)
    clang_cmd = clang_cmd + files
    logging.debug("clang-tidy cmd: %s", " ".join(clang_cmd))
    clang_stdout, clang_stderr, clang_rc = run_command(clang_cmd)
    clang_tidy_output = clang_stdout + clang_stderr

    clang_tidy_log = os.path.join(args.log_dir, "clang-tidy.log")
    write_file(clang_tidy_log, clang_tidy_output)
    emit_problem_matchers(clang_tidy_log, "clang.json", "clang")
    if clang_rc != 0:
        logging.error("clang-tidy failed (exit=%s)", clang_rc)
        sys.exit(clang_rc)

    error_pattern = r"^.+:\d+:\d+: error: .+$"
    warning_pattern = r"^.+:\d+:\d+: warning: .+$"
    note_pattern = r"^.+:\d+:\d+: note: .+$"

    errors_count = count_occurrences(error_pattern, clang_tidy_output)
    warnings_count = count_occurrences(warning_pattern, clang_tidy_output)
    notes_count = count_occurrences(note_pattern, clang_tidy_output)

    logging.info(
        "Found %d errors, %d warnings, %d notes",
        errors_count,
        warnings_count,
        notes_count,
    )

    report = generate_markdown_report(
        errors_count, warnings_count, notes_count, clang_tidy_output, enabled_output
    )
    append_file(args.report_file, report)

    sys.exit(0 if errors_count == 0 else 1)


if __name__ == "__main__":
    main()
