#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
from utils import run_command, setup_logger


def count_clang_format_errors(log_text: str) -> int:
    """
    Count lines that indicate clang-format violations.
    The pattern looks for lines ending with "[-Wclang-format-violations]".
    """
    pattern = r"\[-Wclang-format-violations\]$"
    matches = re.findall(pattern, log_text, re.MULTILINE)
    logging.debug(f"Matches found: {matches}")
    return len(matches)


def write_log_file(log_file: str, content: str):
    """Write content to the specified log file."""
    with open(log_file, "w", encoding="utf-8") as f:
        f.write(content)
    logging.info(f"Log written to {log_file}")


def generate_markdown_report(clang_format_errors: int, output: str) -> str:
    """Generate a Markdown report section for the clang-format output."""
    report_lines = []
    if clang_format_errors > 0:
        report_lines.append(
            f"<details><summary>:pencil2: Clang-format would reformat {clang_format_errors} file(s)</summary>"
        )
    else:
        report_lines.append(
            "<details><summary>:heavy_check_mark: Clang-format would reformat no file</summary>"
        )
    report_lines.append("")
    report_lines.append("````")
    report_lines.append(output)
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    return "\n".join(report_lines)


def append_report(report_file: str, report: str):
    """Append the generated Markdown report to the given report file."""
    with open(report_file, "a", encoding="utf-8") as rf:
        rf.write(report + "\n")
    logging.info(f"Report appended to {report_file}")


def emit_clang_format_problem_matchers(log_path: str):
    """Emit GitHub Actions problem matcher commands for clang-format output."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/clang.json")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print("::remove-matcher owner=clang::")


def main():
    parser = argparse.ArgumentParser(
        description="Run clang-format in dry-run mode on C++ files and append a Markdown report."
    )
    parser.add_argument(
        "--files",
        required=True,
        help="A space-separated list or glob pattern of C++ files to check.",
    )
    parser.add_argument(
        "--clang-style",
        required=True,
        help="Clang-format style (for example, 'file' to use .clang-format or a specific style).",
    )
    parser.add_argument(
        "--log-dir", required=True, help="Directory where the log file will be written."
    )
    parser.add_argument(
        "--report-file",
        required=True,
        help="Path to the Markdown report file to append results.",
    )
    parser.add_argument(
        "--workspace-dir",
        required=True,
        help="Path to the Markdown report file to append results.",
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output.")
    args = parser.parse_args()

    # Setup logging.
    setup_logger(args.verbose)

    # Ensure directories exist.
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    # Prepare log file location.
    log_file = os.path.join(args.log_dir, "clang-format.log")

    # Build the clang-format command.
    cmd = f"clang-format --dry-run --ferror-limit=1 --verbose --style={args.clang_style} {args.files}"
    logging.info(f"Running command: {cmd}")
    stdout, stderr, exit_code = run_command(cmd)
    output = stdout + stderr

    # Write output to the log file.
    write_log_file(log_file, output)
    logging.info(f"Command exited with code {exit_code}")

    # Emit problem matcher commands so GitHub Actions can highlight clang-format issues.
    emit_clang_format_problem_matchers(log_file)

    # Count clang-format errors.
    clang_format_errors = count_clang_format_errors(output)
    logging.info(f"Found {clang_format_errors} clang-format errors")
    print(f"Found {clang_format_errors} clang-format errors")

    # Generate and append Markdown report.
    report = generate_markdown_report(clang_format_errors, output)
    append_report(args.report_file, report)

    # Exit with 0 if no errors, nonzero otherwise.
    sys.exit(0 if clang_format_errors == 0 else 1)


if __name__ == "__main__":
    main()
