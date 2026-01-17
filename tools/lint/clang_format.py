#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
import shutil
from utils import (
    run_command,
    init_environment,
    append_file,
    emit_problem_matchers,
    write_file,
    add_common_arguments,
    expand_files,
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
            f"<details><summary>:pencil2: Clang-format found"
            f" {clang_format_errors} formatting violation(s)</summary>"
        )
    else:
        report_lines.append(
            "<details><summary>:heavy_check_mark: Clang-format found no formatting violations</summary>"
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
        default="file",
        help="Clang-format style (e.g., 'file' to use .clang-format or a specific style).",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Apply formatting changes in-place (default: dry-run check).",
    )
    args = parser.parse_args()
    init_environment(args)

    args.clang_style = (args.clang_style or "file").strip() or "file"

    if shutil.which("clang-format") is None:
        logging.error("clang-format not found in PATH")
        sys.exit(127)

    files = expand_files(args.files)
    if not files:
        sys.exit(0)
    # Always run a dry-run pass first to collect a useful report (and count issues).
    check_cmd = [
        "clang-format",
        "--dry-run",
        "--ferror-limit=1",
        "--verbose",
        f"--style={args.clang_style}",
        *files,
    ]
    stdout, stderr, rc = run_command(check_cmd)
    output_check = stdout + "\n" + stderr

    clang_format_errors = count_clang_format_errors(output_check)
    logging.info("Found %s clang-format violations", clang_format_errors)

    output = output_check

    if args.apply and clang_format_errors > 0:
        apply_cmd = [
            "clang-format",
            "-i",
            f"--style={args.clang_style}",
            *files,
        ]
        _, _, apply_rc = run_command(apply_cmd)
        if apply_rc != 0:
            logging.error("clang-format apply failed (exit=%s)", apply_rc)
            sys.exit(apply_rc)

        # Re-check after applying so `--apply` can succeed when it fixed everything.
        stdout2, stderr2, rc2 = run_command(check_cmd)
        output_after = stdout2 + "\n" + stderr2
        clang_format_errors = count_clang_format_errors(output_after)
        logging.info("After apply: %s clang-format violations", clang_format_errors)
        rc = rc2
        output = output_after

    log_file = os.path.join(args.log_dir, "clang-format.log")
    write_file(log_file, output)
    emit_problem_matchers(log_file, "clang.json", "clang")

    report = generate_markdown_report(clang_format_errors, output)
    append_file(args.report_file, report)

    if rc != 0 and clang_format_errors == 0:
        logging.error("clang-format failed (exit=%s)", rc)
        sys.exit(rc)

    sys.exit(0 if clang_format_errors == 0 else 1)


if __name__ == "__main__":
    main()
