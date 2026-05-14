#!/usr/bin/env python3
import argparse
import logging
import os
import shlex
import subprocess
import sys

from utils import append_file, init_environment


def run_check_command(log_dir: str, repo_root: str) -> int:
    cmd = [
        "python3",
        "src/Tools/typing/generate_stubs.py",
        "check",
        "--root",
        ".",
        "--out-dir",
        "src/Tools/typing/generated",
        "--log-dir",
        log_dir,
    ]
    logging.info("Running Python stub checks: %s", shlex.join(cmd))
    result = subprocess.run(cmd, cwd=repo_root)
    if result.returncode != 0:
        logging.error("Python stub checks failed with exit code %s", result.returncode)
    return result.returncode


def read_file(path: str) -> str:
    if not os.path.exists(path):
        return "(no log written)\n"
    with open(path, "r", encoding="utf-8") as handle:
        return handle.read()


def report_section(summary: str, title: str, log_path: str) -> str:
    lines = [f"<details><summary>{summary}</summary>", "", f"`{title}`", "````"]
    lines.append(read_file(log_path))
    lines.extend(["````", "</details>", ""])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Run the Python binding stub generator and smoke type checks."
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
    init_environment(args)

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    exit_code = run_check_command(args.log_dir, repo_root)

    section_title = (
        ":heavy_check_mark: Generated binding stub smoke checks passed"
        if exit_code == 0
        else ":fire: Generated binding stub smoke checks failed"
    )
    report = [
        f"<details><summary>{section_title}</summary>",
        "",
        report_section(
            "Stub generation",
            "Stub generation",
            os.path.join(args.log_dir, "python-stubs-generate.log"),
        ),
        report_section(
            "Pyright smoke check", "Pyright", os.path.join(args.log_dir, "python-stubs-pyright.log")
        ),
        report_section(
            "Pyrefly smoke check", "Pyrefly", os.path.join(args.log_dir, "python-stubs-pyrefly.log")
        ),
        "</details>",
        "",
    ]
    append_file(args.report_file, "\n".join(report))
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
