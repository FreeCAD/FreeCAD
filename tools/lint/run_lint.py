#!/usr/bin/env python3
import argparse
import subprocess
import sys
import os
import logging
from utils import run_command, setup_logger


def change_to_repo_root() -> str:
    """Change working directory to the repository root and return its path."""
    script_dir = os.path.dirname(os.path.realpath(__file__))
    repo_root = os.path.abspath(os.path.join(script_dir, "..", ".."))
    os.chdir(repo_root)
    logging.debug(f"Changed working directory to {repo_root}")
    return repo_root


def get_changed_files(base_commit: str) -> list:
    """Return a list of changed files compared to the given base commit."""
    try:
        stdout, _, _ = run_command(
            ["git", "diff", "--name-only", base_commit], check=True
        )
        logging.debug(f"git diff output:\n{stdout}")
        files = stdout.strip().splitlines()
        # Only return files that currently exist (to avoid deleted files).
        valid_files = [f for f in files if os.path.isfile(f)]
        logging.debug(f"Valid changed files: {valid_files}")
        return valid_files
    except subprocess.CalledProcessError:
        logging.error("Unable to determine changed files from Git.")
        sys.exit(1)


def get_merge_base(target_branch: str) -> str:
    """Return the merge base between HEAD and the target branch."""
    try:
        stdout, _, _ = run_command(
            ["git", "merge-base", "HEAD", target_branch], check=True
        )
        merge_base = stdout.strip()
        logging.debug(f"Merge base for {target_branch}: {merge_base}")
        return merge_base
    except subprocess.CalledProcessError:
        logging.error(f"Unable to determine merge base with {target_branch}.")
        sys.exit(1)


def run_lint_script(script_path: str, files: list, extra_args: str = "") -> int:
    """Run a lint script with the provided file list and extra arguments."""
    if not files:
        print(f"Skipping {script_path}: no files provided.")
        return 0
    file_list = " ".join(files)
    command = f'python3 {script_path} --files "{file_list}" {extra_args}'
    logging.debug(f"Running lint script with command: {command}")
    result = subprocess.run(command, shell=True)
    logging.debug(f"Lint script exit code: {result.returncode}")
    return result.returncode


def main():
    parser = argparse.ArgumentParser(
        description="Run local lint scripts on files changed in Git."
    )
    parser.add_argument(
        "--base",
        default="HEAD",
        help="Git base commit to compare changes (default: HEAD).",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Run lint on all tracked files instead of only changed files.",
    )
    parser.add_argument(
        "--from-merge-base",
        action="store_true",
        help="Run linting on changes from the merge base of HEAD and the target branch (e.g. origin/main).",
    )
    parser.add_argument(
        "--target-branch",
        default="origin/main",
        help="The target branch to use with --from-merge-base (default: origin/main).",
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output.")
    args = parser.parse_args()
    setup_logger(args.verbose)

    # Change working directory to repository root.
    change_to_repo_root()

    # Determine which files to lint.
    if args.all:
        logging.debug("Running in all-files mode.")
        stdout, _, _ = run_command(["git", "ls-files"], check=True)
        changed_files = stdout.strip().splitlines()
    elif args.from_merge_base:
        base_commit = get_merge_base(args.target_branch)
        print(f"Using merge base {base_commit} from {args.target_branch}")
        changed_files = get_changed_files(base_commit)
    else:
        changed_files = get_changed_files(args.base)

    if not changed_files:
        print("No changed files found.")
        sys.exit(0)

    # For generic formatting checks, run them on all changed files.
    generic_files = changed_files

    # Define locations for logs and the report.
    lint_dir = os.path.join("tools", "lint")
    log_dir = os.path.join("/tmp", "lint_logs")
    report_file = os.path.join("/tmp", "lint_report", "lint-report.md")

    os.makedirs(log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(report_file), exist_ok=True)

    # Truncate (or create) the report file so it doesn't simply append.
    with open(report_file, "w", encoding="utf-8") as f:
        f.write("")

    exit_code = 0

    print("\n=== Running Generic Formatting Checks ===")
    exit_code |= run_lint_script(
        os.path.join(lint_dir, "generic_checks.py"),
        generic_files,
        "--lineendings-check --whitespace-check --tabs-check "
        + f"--report-file {report_file} --log-dir {log_dir}",
    )

    print("\n=== Running Python Lint Checks ===")
    python_files = [f for f in changed_files if f.endswith(".py") or f.endswith(".pyi")]
    if python_files:
        exit_code |= run_lint_script(
            os.path.join(lint_dir, "pylint.py"),
            python_files,
            f"--disable C0302,C0303 --report-file {report_file} --log-dir {log_dir}",
        )
        print("\n=== Running Python Format Checks ===")
        exit_code |= run_lint_script(
            os.path.join(lint_dir, "black.py"),
            python_files,
            f"--report-file {report_file} --log-dir {log_dir}",
        )
    else:
        print("No Python files to lint.")

    cpp_files = [
        f
        for f in changed_files
        if f.endswith(".h")
        or f.endswith(".hpp")
        or f.endswith(".c")
        or f.endswith(".cpp")
    ]

    print("\n=== Running C++ Lint Checks ===")
    if cpp_files:
        exit_code |= run_lint_script(
            os.path.join(lint_dir, "cpplint.py"),
            cpp_files,
            f"--report-file {report_file} --log-dir {log_dir}",
        )
    else:
        print("No C++ files to lint.")

    print("\nLocal linting complete. Report generated at:", report_file)
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
