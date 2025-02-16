#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
from typing import Tuple
from utils import run_command, setup_logger

DEFAULT_LINE_LENGTH_LIMIT = 100

def install_black():
    """Install Black if it isn't already installed."""
    logging.info("Installing Black (if needed)...")
    run_command(["pip", "install", "-q", "black"], check=True)


def run_black(files: str, line_length: str, log_dir: str) -> Tuple[str, int, str]:
    """
    Run Black in check mode on the provided files.

    Returns:
        log_file: Path to the log file where output is stored.
        exit_code: The exit code from the Black command.
        combined_output: The combined stdout and stderr from Black.
    """
    log_file = os.path.join(log_dir, "black.log")
    cmd = ["black", "--line-length", str(line_length), "--check"] + files.split()
    logging.info(f"Running Black command: {' '.join(cmd)}")
    stdout, stderr, exit_code = run_command(cmd, check=False)
    combined_output = stdout + "\n" + stderr
    with open(log_file, "w", encoding="utf-8") as f:
        f.write(combined_output)
    logging.info(f"Black exit code: {exit_code}")
    return log_file, exit_code, combined_output


def parse_black_output(output: str) -> Tuple[int, int]:
    """
    Parse Black output to determine how many files would be reformatted and how many would fail.

    Returns:
        A tuple (black_reformats, black_fails).
    """
    black_reformats = 0
    black_fails = 0
    # Look for a pattern like: "X files would be reformatted"
    reformats_matches = re.findall(r"(\d+)(?=\s+fil.+ would be reformatted)", output)
    if reformats_matches:
        black_reformats = int(reformats_matches[0])
    # Look for a pattern like: "Y files would fail to reformat"
    fails_matches = re.findall(r"(\d+)(?=\s+fil.+ would fail to reformat)", output)
    if fails_matches:
        black_fails = int(fails_matches[0])
    logging.debug(
        f"Parsed Black output: {black_reformats} files would be reformatted, {black_fails} files would fail"
    )
    return black_reformats, black_fails


def generate_markdown_report(
    black_reformats: int, black_fails: int, log_file: str
) -> str:
    """Generate a Markdown report section based on Black results and log file."""
    report_lines = []
    if black_reformats > 0 or black_fails > 0:
        report_lines.append(
            f"<details><summary>:pencil2: Black would reformat {black_reformats} file(s) and {black_fails} file(s) would fail to reformat</summary>"
        )
    else:
        report_lines.append(
            "<details><summary>:heavy_check_mark: Black would reformat no file</summary>"
        )
    report_lines.append("")
    report_lines.append("````")
    with open(log_file, "r", encoding="utf-8") as f:
        report_lines.append(f.read())
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    return "\n".join(report_lines)


def emit_black_problem_matchers(log_path: str):
    """Emit GitHub Actions problem matcher commands for Black output."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/blackWarning.json")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print("::remove-matcher owner=black-warning::")


def append_report(report_file: str, report: str):
    """Append the generated Markdown report to the specified report file."""
    logging.info(f"Appending report to {report_file}")
    with open(report_file, "a", encoding="utf-8") as f:
        f.write(report + "\n")


def main():
    parser = argparse.ArgumentParser(
        description="Run Black in check mode on Python files and append a Markdown report."
    )
    parser.add_argument(
        "--files",
        required=True,
        help="A space-separated list or glob pattern of Python files to check.",
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
    setup_logger(args.verbose)

    # Ensure necessary directories exist.
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    install_black()

    log_file, exit_code, combined_output = run_black(
        args.files, DEFAULT_LINE_LENGTH_LIMIT, args.log_dir
    )

    # Emit problem matcher commands for Black output.
    emit_black_problem_matchers(log_file)

    black_reformats, black_fails = parse_black_output(combined_output)
    logging.info(
        f"Found {black_reformats} files would be reformatted and {black_fails} files would fail to reformat"
    )
    print(
        f"Found {black_reformats} files would be reformatted and {black_fails} files would fail to reformat"
    )

    report = generate_markdown_report(black_reformats, black_fails, log_file)
    append_report(args.report_file, report)

    sys.exit(0 if exit_code == 0 else 1)


if __name__ == "__main__":
    main()
