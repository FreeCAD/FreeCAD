#!/usr/bin/env python3
import argparse
import sys
import os
import re
import logging
from pathlib import Path
from utils import (
    run_command,
    init_environment,
    write_file,
    append_file,
    emit_problem_matchers,
    add_common_arguments,
    ensure_tool,
    expand_files,
)

DEFAULT_LINE_LENGTH_LIMIT = 100

def load_cpplint_filters(conf_file: str | None = None) -> str:
    """Load cpplint filters from a configuration file.

    By default, resolves `cpplint.cfg` relative to this script (tools/lint/cpplint.cfg),
    so it works regardless of the current working directory.
    """
    if conf_file is None:
        conf_path = Path(__file__).resolve().parent / "cpplint.cfg"
    else:
        conf_path = Path(conf_file)
        if not conf_path.is_absolute():
            conf_path = (Path(__file__).resolve().parent / conf_path).resolve()

    try:
        raw = conf_path.read_text(encoding="utf-8", errors="replace")
    except Exception as exc:
        logging.warning("Could not load %s: %s", conf_path, exc)
        return ""

    parts: list[str] = []
    for line in raw.splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        parts.append(s)
    return "".join(parts).strip()

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

    ensure_tool("cpplint", package="cpplint", prefer_pipx=False, check_args=["--help"])

    cpplint_filters = load_cpplint_filters()

    aggregated_output = ""
    tool_failed = False
    files = expand_files(args.files)
    if not files:
        sys.exit(0)
    for file in files:
        cmd = [
            "cpplint",
            f"--filter={cpplint_filters}",
            f"--linelength={DEFAULT_LINE_LENGTH_LIMIT}",
            file,
        ]
        stdout, stderr, rc = run_command(cmd)
        aggregated_output += stdout + stderr + "\n"
        # cpplint typically returns 0/1 for lint results; 2+ is usually a usage/tool error.
        if rc not in (0, 1):
            tool_failed = True

    cpplint_log = os.path.join(args.log_dir, "cpplint.log")
    write_file(cpplint_log, aggregated_output)
    emit_problem_matchers(cpplint_log, "cpplint.json", "cpplint")

    errors_count = count_cpplint_errors(aggregated_output)
    logging.info(f"Found {errors_count} cpplint errors")

    report = generate_markdown_report(aggregated_output, errors_count)
    append_file(args.report_file, report)

    if tool_failed:
        sys.exit(2)
    sys.exit(0 if errors_count == 0 else 1)

if __name__ == "__main__":
    main()
