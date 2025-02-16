#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
from utils import run_command, setup_logger

def write_file(file_path: str, content: str):
    """Write content to the specified file."""
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(content)
    logging.info("Wrote file: %s", file_path)

def append_file(file_path: str, content: str):
    """Append content to the specified file."""
    with open(file_path, "a", encoding="utf-8") as f:
        f.write(content + "\n")
    logging.info("Appended content to file: %s", file_path)

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
            f"<details><summary>:fire: Clang-Tidy found :fire: {errors_count} errors, :warning: {warnings_count} warnings and :pencil2: {notes_count} notes</summary>"
        )
    elif warnings_count > 0:
        report_lines.append(
            f"<details><summary>:warning: Clang-Tidy found :warning: {warnings_count} warnings and :pencil2: {notes_count} notes</summary>"
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

def count_occurrences(pattern: str, text: str, verbose: bool = False) -> int:
    """Count all occurrences of a regex pattern in text using MULTILINE mode."""
    matches = re.findall(pattern, text, re.MULTILINE)
    if verbose:
        logging.info("Pattern '%s' found %d matches", pattern, len(matches))
    return len(matches)

def run_enabled_checks(clang_style: str, build_dir: str, verbose: bool) -> str:
    """
    Run clang-tidy with --explain-config to list enabled checks.
    Builds the command as a list.
    """
    cmd = [
        "clang-tidy",
        "--quiet",
        f"--format-style={clang_style}",
        "--export-fixes=clang-tidy.yaml",
        "-p",
        build_dir,
        "--explain-config",
    ]
    if verbose:
        logging.info("Running enabled-checks command: %s", " ".join(cmd))
    stdout, stderr, _ = run_command(cmd, capture_output=True, check=True, text=True)
    return stdout + stderr

def run_clang_tidy(files: str, clang_style: str, build_dir: str, verbose: bool) -> str:
    """
    Run clang-tidy on the provided C++ files.
    Splits the files argument into a list.
    """
    files_list = files.split()  # Assumes space-separated file names.
    cmd = [
        "clang-tidy",
        "--quiet",
        f"--format-style={clang_style}",
        "--export-fixes=clang-tidy.yaml",
        "-p",
        build_dir,
    ] + files_list
    if verbose:
        logging.info("Running clang-tidy command: %s", " ".join(cmd))
    stdout, stderr, _ = run_command(cmd, capture_output=True, check=True, text=True)
    return stdout + stderr

def emit_clang_tidy_problem_matchers(log_path: str):
    """Emit GitHub Actions problem matcher commands for clang-tidy output."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/clang.json")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print("::remove-matcher owner=clang::")

def main():
    parser = argparse.ArgumentParser(
        description="Run clang-tidy on provided C++ files and append a Markdown report."
    )
    parser.add_argument(
        "--files",
        required=True,
        help="A space-separated list or glob pattern of C++ files to check.",
    )
    parser.add_argument(
        "--clang-style",
        required=True,
        help="Clang-format style (e.g., 'file' to use .clang-format or a specific style).",
    )
    parser.add_argument(
        "--log-dir", required=True, help="Directory where log files will be written."
    )
    parser.add_argument(
        "--report-file",
        required=True,
        help="Path to the Markdown report file to append results.",
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output.")
    args = parser.parse_args()
    verbose = args.verbose

    # Set up logging using the provided utils.
    setup_logger(verbose)

    # Ensure required directories exist.
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    # Define log file paths and the assumed build directory.
    enabled_checks_log = os.path.join(args.log_dir, "clang-tidy-enabled-checks.log")
    clang_tidy_log = os.path.join(args.log_dir, "clang-tidy.log")
    build_dir = "build"  # Assumed build directory

    # Run clang-tidy to list enabled checks.
    enabled_output = run_enabled_checks(args.clang_style, build_dir, verbose)
    write_file(enabled_checks_log, enabled_output)

    # Run clang-tidy on the provided files.
    clang_tidy_output = run_clang_tidy(args.files, args.clang_style, build_dir, verbose)
    write_file(clang_tidy_log, clang_tidy_output)

    # Emit problem matcher commands so GitHub Actions can highlight clang-tidy issues.
    emit_clang_tidy_problem_matchers(clang_tidy_log)

    # Define regex patterns for errors, warnings, and notes.
    error_pattern = r"^.+:\d+:\d+: error: .+$"
    warning_pattern = r"^.+:\d+:\d+: warning: .+$"
    note_pattern = r"^.+:\d+:\d+: note: .+$"

    # Count occurrences in the clang-tidy output.
    errors_count = count_occurrences(error_pattern, clang_tidy_output, verbose)
    warnings_count = count_occurrences(warning_pattern, clang_tidy_output, verbose)
    notes_count = count_occurrences(note_pattern, clang_tidy_output, verbose)

    logging.info(
        "Found %d errors, %d warnings, %d notes",
        errors_count,
        warnings_count,
        notes_count,
    )
    print(
        f"Found {errors_count} errors, {warnings_count} warnings, {notes_count} notes"
    )

    # Generate and append the Markdown report.
    report = generate_markdown_report(
        errors_count, warnings_count, notes_count, clang_tidy_output, enabled_output
    )
    append_file(args.report_file, report)

    # Exit with 0 if no errors found; otherwise, nonzero.
    sys.exit(0 if errors_count == 0 else 1)

if __name__ == "__main__":
    main()
