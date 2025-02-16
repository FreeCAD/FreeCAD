#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
from utils import run_command, setup_logger


def load_cpplint_filters(conf_file="cpplint.cf"):
    """Load cpplint filters from a configuration file."""
    try:
        with open(conf_file, "r", encoding="utf-8") as f:
            # Read and strip any surrounding whitespace
            filters = f.read().strip()
            return filters
    except Exception as e:
        print(f"Warning: Could not load {conf_file}: {e}")
        return ""


def count_cpplint_errors(log_text: str) -> int:
    """
    Count the number of cpplint error/warning lines.
    This uses the same idea as: grep -nIHE "\[[0-9]\]$" in the shell.
    """
    matches = re.findall(r"\[[0-9]+\]$", log_text, re.MULTILINE)
    return len(matches)


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


def emit_cpplint_problem_matchers(log_path: str):
    """Emit GitHub Actions problem matcher commands for cpplint output."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/cpplint.json")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print("::remove-matcher owner=cpplint::")


def main():
    parser = argparse.ArgumentParser(
        description="Run cpplint on provided C++ files and append a Markdown report."
    )
    parser.add_argument(
        "--files",
        required=True,
        help="Space-separated list (or glob-expanded string) of C++ files to lint.",
    )
    parser.add_argument(
        "--line-length",
        default="100",
        help="Line length parameter for cpplint.",
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

    # Ensure directories exist.
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    # Install cpplint quietly.
    run_command(["pip", "install", "-q", "cpplint"], verbose=verbose)

    # Always load filters from file.
    cpplint_filters = load_cpplint_filters("cpplint.cfg")

    cpplint_log = os.path.join(args.log_dir, "cpplint.log")
    aggregated_output = ""
    # Split the files string into a list (assumes space-separated).
    file_list = args.files.split()

    # Run cpplint on each file and accumulate output.
    for file in file_list:
        cmd = [
            "cpplint",
            f"--filters={cpplint_filters}",
            f"--linelength={args.line_length}",
            file,
        ]
        stdout, stderr, _ = run_command(cmd, verbose=verbose)
        aggregated_output += stdout + stderr + "\n"

    # Write the aggregated output to the log file.
    write_file(cpplint_log, aggregated_output)

    # Emit problem matcher commands so GitHub Actions can highlight cpplint issues.
    emit_cpplint_problem_matchers(cpplint_log)

    # Count errors using a regex.
    errors_count = count_cpplint_errors(aggregated_output)
    print(f"Found {errors_count} cpplint errors")

    # Build a Markdown report section.
    report_lines = []
    if errors_count > 0:
        report_lines.append(
            f"<details><summary>:warning: CppLint found {errors_count} errors / warnings</summary>"
        )
    else:
        report_lines.append(
            "<details><summary>:heavy_check_mark: No cpplint errors found </summary>"
        )
    report_lines.append("")
    report_lines.append("````")
    report_lines.append(aggregated_output)
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")

    # Append the report to the report file.
    append_file(args.report_file, "\n".join(report_lines))

    # Exit with 0 if no errors, else nonzero.
    sys.exit(0 if errors_count == 0 else 1)


if __name__ == "__main__":
    main()
