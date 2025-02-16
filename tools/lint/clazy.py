#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging

from utils import run_command, setup_logger

def emit_clazy_problem_matchers(log_path: str):
    """Emit GitHub Actions problem matcher commands for clazy output."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/clang.json")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print("::remove-matcher owner=clang::")

def generate_markdown_report(aggregated_output: str, errors_count: int, warnings_count: int, notes_count: int) -> str:
    """Generate a Markdown report based on clazy output and issue counts."""
    report_lines = []
    if errors_count > 0:
        report_lines.append(f"<details><summary>:fire: Clazy found :fire: {errors_count} errors, :warning: {warnings_count} warnings and :pencil2: {notes_count} notes</summary>")
    elif warnings_count > 0:
        report_lines.append(f"<details><summary>:warning: Clazy found :warning: {warnings_count} warnings and :pencil2: {notes_count} notes</summary>")
    elif notes_count > 0:
        report_lines.append(f"<details><summary>:pencil2: Clazy found :pencil2: {notes_count} notes</summary>")
    else:
        report_lines.append("<details><summary>:heavy_check_mark: Clazy found no errors, warnings or notes</summary>")
    report_lines.append("")
    report_lines.append("[List of checks](https://github.com/KDE/clazy#list-of-checks), [This explains some of the clazy warnings](https://www.kdab.com/uncovering-32-qt-best-practices-compile-time-clazy/)")
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
    parser.add_argument("--files", required=True,
                        help="A space-separated list (or glob-expanded string) of C++ files to check.")
    parser.add_argument("--clazy-checks", required=True,
                        help="Comma-separated list of clazy checks to run.")
    parser.add_argument("--log-dir", required=True,
                        help="Directory where the log file will be written.")
    parser.add_argument("--report-file", required=True,
                        help="Path to the Markdown report file to append results.")
    parser.add_argument("--verbose", action="store_true",
                        help="Enable verbose output.")
    args = parser.parse_args()

    # Set up logging according to the verbose flag.
    setup_logger(args.verbose)

    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    log_file = os.path.join(args.log_dir, "clazy.log")
    # Assume the build compile_commands.json is in build/
    build_compile_commands = "build/compile_commands.json"
    fix_file = "clazy.yaml"  # For exported fixes (not further used here)

    aggregated_output = ""
    file_list = args.files.split()

    for file in file_list:
        cmd = [
            "clazy-standalone",
            f"--export-fixes={fix_file}",
            f"-checks={args.clazy_checks}",
            "-p", build_compile_commands,
            file
        ]
        logging.debug("Running: " + " ".join(cmd))
        stdout, stderr, _ = run_command(cmd)
        aggregated_output += stdout + stderr + "\n"

    with open(log_file, "w", encoding="utf-8") as f:
        f.write(aggregated_output)

    # Emit problem matcher commands so GitHub Actions can highlight clazy issues.
    emit_clazy_problem_matchers(log_file)

    # Count errors, warnings, and notes using regex.
    error_pattern   = r"^.+:\d+:\d+: error: .+$"
    warning_pattern = r"^.+:\d+:\d+: warning: .+$"
    note_pattern    = r"^.+:\d+:\d+: note: .+$"
    errors_count   = len(re.findall(error_pattern, aggregated_output, re.MULTILINE))
    warnings_count = len(re.findall(warning_pattern, aggregated_output, re.MULTILINE))
    notes_count    = len(re.findall(note_pattern, aggregated_output, re.MULTILINE))

    logging.info(f"Found {errors_count} errors, {warnings_count} warnings, {notes_count} notes")

    # Generate and append the Markdown report.
    report = generate_markdown_report(aggregated_output, errors_count, warnings_count, notes_count)
    with open(args.report_file, "a", encoding="utf-8") as rf:
        rf.write(report + "\n")

    sys.exit(0 if errors_count == 0 else 1)

if __name__ == "__main__":
    main()
