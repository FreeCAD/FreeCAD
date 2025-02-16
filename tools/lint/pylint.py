#!/usr/bin/env python3
import argparse
import sys
import re
import os
import logging
from utils import run_command, setup_logger


def install_pylint():
    """Install pylint if not already available."""
    logging.info("Installing pylint (if needed)...")
    run_command(["pip", "install", "-q", "pylint"], check=True)


def write_log_file(path: str, content: str):
    """Write content to a specified log file."""
    logging.info(f"Writing log file: {path}")
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


def get_enabled_checks(log_dir: str) -> str:
    """Run pylint to list enabled messages and write to a log file."""
    enabled_log_path = os.path.join(log_dir, "pylint-enabled-checks.log")
    stdout, _, _ = run_command(["pylint", "--list-msgs-enabled"])
    write_log_file(enabled_log_path, stdout)
    return enabled_log_path


def run_pylint(files: str, disable: str, log_dir: str) -> str:
    """Run pylint on the provided files and log the output."""
    pylint_log_path = os.path.join(log_dir, "pylint.log")
    # Assume files is a space-separated list
    cmd = ["pylint", f"--disable={disable}"] + files.split()
    stdout, stderr, _ = run_command(cmd, check=False)
    combined_output = stdout + "\n" + stderr
    write_log_file(pylint_log_path, combined_output)
    return pylint_log_path


def count_pattern(pattern: str, text: str) -> int:
    """Count numeric values found by a regex pattern in the text."""
    matches = re.findall(pattern, text)
    logging.debug(f"Pattern '{pattern}' found matches: {matches}")
    return sum(int(match) for match in matches) if matches else 0


def parse_pylint_output(log_path: str) -> dict:
    """Parse pylint log file for error, warning, refactor, and convention counts."""
    with open(log_path, "r", encoding="utf-8") as f:
        content = f.read()
    errors = count_pattern(r"error\s+\|(\d+)", content)
    warnings = count_pattern(r"warning\s+\|(\d+)", content)
    refactorings = count_pattern(r"refactor\s+\|(\d+)", content)
    conventions = count_pattern(r"convention\s+\|(\d+)", content)
    logging.info(
        f"Pylint issues - Errors: {errors}, Warnings: {warnings}, Refactorings: {refactorings}, Conventions: {conventions}"
    )
    return {
        "errors": errors,
        "warnings": warnings,
        "refactorings": refactorings,
        "conventions": conventions,
        "full_output": content,
    }


def generate_markdown_report(pylint_counts: dict, enabled_checks_log: str, pylint_log: str) -> str:
    """Generate a Markdown-formatted report based on pylint results and logs."""
    
    def generate_summary(counts: dict) -> str:
        errors = counts.get("errors", 0)
        warnings = counts.get("warnings", 0)
        refactorings = counts.get("refactorings", 0)
        conventions = counts.get("conventions", 0)
        
        if errors:
            return (f":fire: Pylint found :fire: {errors} errors, "
                    f":warning: {warnings} warnings, :construction: {refactorings} refactorings and "
                    f":pencil2: {conventions} conventions")
        elif warnings:
            return (f":warning: Pylint found :warning: {warnings} warnings, :construction: {refactorings} refactorings and "
                    f":pencil2: {conventions} conventions")
        elif refactorings:
            return (f":construction: Pylint found :construction: {refactorings} refactorings and "
                    f":pencil2: {conventions} conventions")
        elif conventions:
            return f":pencil2: Pylint found :pencil2: {conventions} conventions"
        else:
            return ":heavy_check_mark: No pylint errors found"
    
    def read_file_contents(file_path: str) -> str:
        with open(file_path, "r", encoding="utf-8") as f:
            return f.read()
    
    report_lines = []
    summary = generate_summary(pylint_counts)
    
    # Outer details block with the summary.
    report_lines.append(f"<details><summary>{summary}</summary>")
    report_lines.append("")
    
    # Enabled checks log block.
    report_lines.append("<details><summary>:information_source: Enabled checks</summary>")
    report_lines.append("")
    report_lines.append("````")
    report_lines.append(read_file_contents(enabled_checks_log))
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    
    # Pylint log block.
    report_lines.append("<details>")
    report_lines.append("````")
    report_lines.append(read_file_contents(pylint_log))
    report_lines.append("````")
    report_lines.append("</details>")
    
    # Close the outer details block.
    report_lines.append("</details>")
    
    return "\n".join(report_lines)


def append_report(report_file: str, report: str):
    """Append the generated Markdown report to the specified file."""
    logging.info(f"Appending report to {report_file}")
    with open(report_file, "a", encoding="utf-8") as f:
        f.write(report + "\n")


def emit_pylint_problem_matchers(log_path: str):
    """Emit GitHub Actions problem matcher commands for pylint output."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        # Emit problem matcher add commands.
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/pylintError.json")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/pylintWarning.json")
        # Output the content of the pylint log to the console.
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        # Emit commands to remove the matchers.
        print("::remove-matcher owner=pylint-error::")
        print("::remove-matcher owner=pylint-warning::")


def main():
    parser = argparse.ArgumentParser(
        description="Run Pylint on provided Python files and append a Markdown report."
    )
    parser.add_argument(
        "--files",
        required=True,
        help="A space-separated list (or glob) of Python files to lint.",
    )
    parser.add_argument(
        "--disable",
        default="C0302,C0303",
        help="Comma-separated list of pylint checks to disable.",
    )
    parser.add_argument(
        "--log-dir", required=True, help="Directory in which to write log files."
    )
    parser.add_argument(
        "--report-file",
        required=True,
        help="Path to the Markdown report file to append results.",
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output.")
    args = parser.parse_args()

    setup_logger(args.verbose)

    # Ensure directories exist.
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    # Install pylint if needed.
    install_pylint()

    # Generate and log enabled checks.
    enabled_checks_log = get_enabled_checks(args.log_dir)

    # Run pylint on provided files.
    pylint_log = run_pylint(args.files, args.disable, args.log_dir)

    # Emit problem matcher commands for GitHub Actions.
    emit_pylint_problem_matchers(pylint_log)

    # Parse pylint log for issue counts.
    pylint_counts = parse_pylint_output(pylint_log)

    # Generate Markdown report.
    report = generate_markdown_report(pylint_counts, enabled_checks_log, pylint_log)

    # Append the generated report to the report file.
    append_report(args.report_file, report)

    # Output a summary and exit accordingly.
    summary_msg = (
        f"Found {pylint_counts['errors']} errors, {pylint_counts['warnings']} warnings, "
        f"{pylint_counts['refactorings']} refactorings, {pylint_counts['conventions']} conventions"
    )
    logging.info(summary_msg)
    sys.exit(0 if pylint_counts["errors"] == 0 else 1)


if __name__ == "__main__":
    main()
