#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging

from utils import run_command, setup_logger

def count_occurrences(pattern, text, verbose=False):
    matches = re.findall(pattern, text, re.MULTILINE)
    if verbose:
        logging.debug(f"Found {len(matches)} matches for pattern: {pattern}")
    return len(matches)

def emit_clazy_qt6_problem_matchers(log_path: str):
    """Emit GitHub Actions problem matcher commands for clazy QT6 output."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/clang.json")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print("::remove-matcher owner=clang::")

def generate_markdown_report(output: str, errors_count: int, warnings_count: int, notes_count: int) -> str:
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
    parser.add_argument("--files", required=True,
                        help="A space-separated list (or glob-expanded string) of C++ files to check.")
    parser.add_argument("--clazy-qt6-checks", required=True,
                        help="Comma-separated list of clazy QT6 checks to run.")
    parser.add_argument("--log-dir", required=True,
                        help="Directory where the log file will be written.")
    parser.add_argument("--report-file", required=True,
                        help="Path to the Markdown report file to append results.")
    parser.add_argument("--verbose", action="store_true",
                        help="Enable verbose output.")
    args = parser.parse_args()
    verbose = args.verbose

    setup_logger(verbose)

    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    log_file = os.path.join(args.log_dir, "clazyQT6.log")
    build_dir = "build"  # Assumed build directory
    fix_file = "clazyQT6.yaml"

    # Build the command as a list instead of a shell string.
    files = args.files.split()
    cmd = [
        "clazy-standalone",
        f"--export-fixes={fix_file}",
        f"-checks={args.clazy_qt6_checks}",
        "-p", build_dir
    ] + files

    logging.debug("Running command: " + " ".join(cmd))
    stdout, stderr, _ = run_command(cmd)
    output = stdout + stderr

    with open(log_file, "w", encoding="utf-8") as f:
        f.write(output)

    # Emit problem matcher commands so GitHub Actions can highlight clazy QT6 issues.
    emit_clazy_qt6_problem_matchers(log_file)

    error_pattern   = r"^.+:\d+:\d+: error: .+$"
    warning_pattern = r"^.+:\d+:\d+: warning: .+$"
    note_pattern    = r"^.+:\d+:\d+: note: .+$"
    errors_count   = count_occurrences(error_pattern, output, verbose=verbose)
    warnings_count = count_occurrences(warning_pattern, output, verbose=verbose)
    notes_count    = count_occurrences(note_pattern, output, verbose=verbose)

    logging.info(f"Found {errors_count} errors, {warnings_count} warnings, {notes_count} notes")

    # Generate and append the Markdown report.
    report = generate_markdown_report(output, errors_count, warnings_count, notes_count)
    with open(args.report_file, "a", encoding="utf-8") as rf:
        rf.write(report + "\n")

    sys.exit(0 if errors_count == 0 else 1)

if __name__ == "__main__":
    main()
