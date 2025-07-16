#!/usr/bin/env python3
import argparse
import sys
import re
import os
import logging
from utils import (
    run_command,
    add_common_arguments,
    init_environment,
    write_file,
    append_file,
    emit_problem_matchers,
)


def get_enabled_checks(log_dir: str) -> str:
    """Run pylint to list enabled messages and write to a log file."""
    enabled_log_path = os.path.join(log_dir, "pylint-enabled-checks.log")
    stdout, _, _ = run_command(["pylint", "--list-msgs-enabled"])
    write_file(enabled_log_path, stdout)
    return enabled_log_path


def run_pylint(files: str, disable: str, log_dir: str) -> str:
    """Run pylint on the provided files and log the output."""
    pylint_log_path = os.path.join(log_dir, "pylint.log")
    cmd = ["pylint", f"--disable={disable}"] + files.split()
    stdout, stderr, _ = run_command(cmd, check=False)
    combined_output = stdout + "\n" + stderr
    write_file(pylint_log_path, combined_output)
    return pylint_log_path


def count_pattern(pattern: str, text: str) -> int:
    """Count numeric values found by a regex pattern in the text."""
    matches = re.findall(pattern, text)
    logging.debug("Pattern '%s' found matches: %s", pattern, matches)
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
        "Pylint issues - Errors: %s, Warnings: %s, Refactorings: %s, Conventions: %s",
        errors,
        warnings,
        refactorings,
        conventions,
    )
    return {
        "errors": errors,
        "warnings": warnings,
        "refactorings": refactorings,
        "conventions": conventions,
        "full_output": content,
    }


def generate_markdown_report(
    pylint_counts: dict, enabled_checks_log: str, pylint_log: str
) -> str:
    """Generate a Markdown-formatted report based on pylint results and logs."""

    def generate_summary(counts: dict) -> str:
        errors = counts.get("errors", 0)
        warnings = counts.get("warnings", 0)
        refactorings = counts.get("refactorings", 0)
        conventions = counts.get("conventions", 0)
        if errors:
            return (
                f":fire: Pylint found :fire: {errors} errors, "
                f":warning: {warnings} warnings, :construction: {refactorings} refactorings and "
                f":pencil2: {conventions} conventions"
            )
        if warnings:
            return (
                f":warning: Pylint found :warning: {warnings} warnings, "
                f":construction: {refactorings} refactorings and "
                f":pencil2: {conventions} conventions"
            )
        if refactorings:
            return (
                f":construction: Pylint found :construction: {refactorings} refactorings and "
                f":pencil2: {conventions} conventions"
            )
        if conventions:
            return f":pencil2: Pylint found :pencil2: {conventions} conventions"
        return ":heavy_check_mark: No pylint errors found"

    def read_file_contents(file_path: str) -> str:
        with open(file_path, "r", encoding="utf-8") as f:
            return f.read()

    report_lines = []
    summary = generate_summary(pylint_counts)
    report_lines.append(f"<details><summary>{summary}</summary>")
    report_lines.append("")
    report_lines.append(
        "<details><summary>:information_source: Enabled checks</summary>"
    )
    report_lines.append("")
    report_lines.append("````")
    report_lines.append(read_file_contents(enabled_checks_log))
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    report_lines.append("<details>")
    report_lines.append("````")
    report_lines.append(read_file_contents(pylint_log))
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("</details>")
    return "\n".join(report_lines)


def main():
    parser = argparse.ArgumentParser(
        description="Run Pylint on provided Python files and append a Markdown report."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--disable",
        default="C0302,C0303",
        help="Comma-separated list of pylint checks to disable.",
    )
    args = parser.parse_args()
    init_environment(args)

    logging.info("Installing pylint (if needed)...")
    run_command(["pip", "install", "-q", "pylint"], check=True)

    enabled_checks_log = get_enabled_checks(args.log_dir)
    pylint_log = run_pylint(args.files, args.disable, args.log_dir)

    emit_problem_matchers(pylint_log, "pylintError.json", "pylint-error")
    emit_problem_matchers(pylint_log, "pylintWarning.json", "pylint-warning")

    pylint_counts = parse_pylint_output(pylint_log)

    summary_msg = (
        f"Found {pylint_counts['errors']} errors, {pylint_counts['warnings']} warnings, "
        f"{pylint_counts['refactorings']} refactorings, {pylint_counts['conventions']} conventions"
    )
    logging.info(summary_msg)

    report = generate_markdown_report(pylint_counts, enabled_checks_log, pylint_log)
    append_file(args.report_file, report)

    sys.exit(0 if pylint_counts["errors"] == 0 else 1)


if __name__ == "__main__":
    main()
