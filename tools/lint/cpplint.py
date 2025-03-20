#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
from utils import (
    run_command,
    init_environment,
    write_file,
    append_file,
    emit_problem_matchers,
    add_common_arguments,
)

DEFAULT_LINE_LENGTH_LIMIT = 100

def load_cpplint_filters(conf_file="cpplint.cfg"):
    """Load cpplint filters from a configuration file."""
    try:
        with open(conf_file, "r", encoding="utf-8") as f:
            filters = f.read().strip()
            return filters
    except Exception as e:
        logging.warning(f"Could not load {conf_file}: {e}")
        return ""

def count_cpplint_errors(log_text: str) -> int:
    """
    Count the number of cpplint error/warning lines.
    """
    matches = re.findall(r"\[[0-9]+\]$", log_text, re.MULTILINE)
    return len(matches)

def generate_markdown_report(aggregated_output: str, errors_count: int) -> str:
    """Generate a Markdown report section based on cpplint output and error count."""
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
    return "\n".join(report_lines)

def main():
    parser = argparse.ArgumentParser(
        description="Run cpplint on provided C++ files and append a Markdown report."
    )
    add_common_arguments(parser)
    args = parser.parse_args()
    init_environment(args)

    run_command(["pip", "install", "-q", "cpplint"], check=True)

    cpplint_filters = load_cpplint_filters("cpplint.cfg")

    aggregated_output = ""
    for file in args.files.split():
        cmd = [
            "cpplint",
            f"--filters={cpplint_filters}",
            f"--linelength={DEFAULT_LINE_LENGTH_LIMIT}",
            file,
        ]
        stdout, stderr, _ = run_command(cmd)
        aggregated_output += stdout + stderr + "\n"

    cpplint_log = os.path.join(args.log_dir, "cpplint.log")
    write_file(cpplint_log, aggregated_output)
    emit_problem_matchers(cpplint_log, "cpplint.json", "cpplint")

    errors_count = count_cpplint_errors(aggregated_output)
    logging.info(f"Found {errors_count} cpplint errors")

    report = generate_markdown_report(aggregated_output, errors_count)
    append_file(args.report_file, report)

    sys.exit(0 if errors_count == 0 else 1)

if __name__ == "__main__":
    main()
