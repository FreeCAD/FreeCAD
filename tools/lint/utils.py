#!/usr/bin/env python3
import os
import sys
import logging
import subprocess
import argparse
from typing import Tuple


def run_command(cmd, check=False) -> Tuple[str, str, int]:
    """
    Run a command using subprocess.run and return stdout, stderr, and exit code.
    """
    try:
        result = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=check, text=True
        )
        return result.stdout, result.stderr, result.returncode
    except subprocess.CalledProcessError as e:
        return e.stdout, e.stderr, e.returncode


def setup_logger(verbose: bool):
    """
    Setup the logging level based on the verbose flag.
    """
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(level=level, format="%(levelname)s: %(message)s")


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


def emit_problem_matchers(log_path: str, matcher_filename: str, remove_owner: str):
    """
    Emit GitHub Actions problem matcher commands using the given matcher file.

    This function will:
      1. Check if the log file exists.
      2. Use the RUNNER_WORKSPACE environment variable to construct the matcher path.
      3. Print the add-matcher command, then the log content, then the remove-matcher command.
    """
    if os.path.isfile(log_path):
        runner_workspace = os.getenv("RUNNER_WORKSPACE")
        matcher_path = os.path.join(
            runner_workspace, "FreeCAD", ".github", "problemMatcher", matcher_filename
        )
        print(f"::add-matcher::{matcher_path}")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print(f"::remove-matcher owner={remove_owner}::")


def add_common_arguments(parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    """
    Add common command-line arguments shared between tools.
    """
    parser.add_argument(
        "--files",
        required=True,
        help="A space-separated list or glob pattern of files to check.",
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
    return parser


def init_environment(args: argparse.Namespace):
    """
    Perform common initialization tasks:
      - Set up logging.
      - Create the log directory.
      - Create the directory for the report file.
    """
    setup_logger(args.verbose)
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)
