#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
from utils import (
    run_command,
    init_environment,
    append_file,
    emit_problem_matchers,
    write_file,
    add_common_arguments,
)


def count_clang_format_errors(log_text: str) -> int:
    """
    Count lines that indicate clang-format violations.
    The pattern looks for lines ending with "[-Wclang-format-violations]".
    """
    pattern = r"\[-Wclang-format-violations\]$"
    matches = re.findall(pattern, log_text, re.MULTILINE)
    logging.debug("Matches found: %s", matches)
    return len(matches)


def generate_markdown_report(clang_format_errors: int, output: str) -> str:
    """Generate a Markdown report section for the clang-format output."""
    report_lines = []
    if clang_format_errors > 0:
        report_lines.append(
            f"<details><summary>:pencil2: Clang-format would reformat"
            f" {clang_format_errors} file(s)</summary>"
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


def main():
    parser = argparse.ArgumentParser(
        description="Run clang-format in dry-run mode on C++ files and append a Markdown report."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--clang-style",
        required=True,
        help="Clang-format style (e.g., 'file' to use .clang-format or a specific style).",
    )
    args = parser.parse_args()
    init_environment(args)

    cmd = (
        f"clang-format --dry-run --ferror-limit=1 --verbose "
        f"--style={args.clang_style} {args.files}"
    )
    stdout, stderr, _ = run_command(cmd)
    output = stdout + "\n" + stderr

    log_file = os.path.join(args.log_dir, "clang-format.log")
    write_file(log_file, output)
    emit_problem_matchers(log_file, "clang.json", "clang")

    clang_format_errors = count_clang_format_errors(output)
    logging.info("Found %s clang-format errors", clang_format_errors)

    report = generate_markdown_report(clang_format_errors, output)
    append_file(args.report_file, report)

    sys.exit(0 if clang_format_errors == 0 else 1)


if __name__ == "__main__":
    main()
