#!/usr/bin/env python3
import argparse
import os
import re
import sys

def parse_args():
    parser = argparse.ArgumentParser(
        description="Check for old Qt string-based connections in files and append a Markdown report."
    )
    parser.add_argument(
        "--files", required=True,
        help="A space-separated list (or glob-expanded string) of files to check for QT string-based connections."
    )
    parser.add_argument(
        "--log-dir", required=True,
        help="Directory where the log file will be written."
    )
    parser.add_argument(
        "--report-file", required=True,
        help="Path to the Markdown report file to append results."
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Enable verbose output."
    )
    return parser.parse_args()

def log_verbose(message, verbose):
    if verbose:
        print(message)

def check_qt_connections(file_path, verbose=False):
    """
    Scan the file for lines containing 'SIGNAL' or 'SLOT' and return a list of matching lines
    with line numbers and a suggestion.
    """
    matches = []
    try:
        with open(file_path, "r", encoding="utf-8", errors="replace") as f:
            for lineno, line in enumerate(f, start=1):
                # Check for SIGNAL or SLOT as whole words.
                if re.search(r'\b(SIGNAL|SLOT)\b', line):
                    # Append formatted match with file, line number and a note.
                    matches.append(f"{file_path}:{lineno}: {line.rstrip()} <-- Consider using functor-based connections")
    except Exception as e:
        log_verbose(f"Error reading {file_path}: {e}", verbose)
    return matches

def emit_qt_problem_matchers(log_path):
    """Emit GitHub Actions problem matcher commands for QT connection issues."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/grepMatcherWarning.json")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print("::remove-matcher owner=grepMatcher-warning::")

def main():
    args = parse_args()
    verbose = args.verbose

    # Expand the file list from a space-separated string.
    file_list = args.files.split()
    log_file = os.path.join(args.log_dir, "qtConnections.log")
    
    # Ensure the log directory and report file's directory exist.
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    all_matches = []
    for file_path in file_list:
        log_verbose(f"Checking file: {file_path}", verbose)
        matches = check_qt_connections(file_path, verbose=verbose)
        all_matches.extend(matches)

    # Write all matches to the log file.
    with open(log_file, "w", encoding="utf-8") as lf:
        lf.write("\n".join(all_matches))

    count = len(all_matches)
    print(f"Found {count} QT string-based connections")

    # Emit problem matcher commands so GitHub Actions can highlight issues.
    emit_qt_problem_matchers(log_file)

    # Append results to the report file.
    with open(args.report_file, "a", encoding="utf-8") as rf:
        if count > 0:
            rf.write(f"<details><summary>:information_source: Found {count} QT string-based connections → consider using QT functor-based connections</summary>\n")
            rf.write("\nFor more information see: https://wiki.qt.io/New_Signal_Slot_Syntax or https://github.com/FreeCAD/FreeCAD/issues/6166\n")
            rf.write("```\n")
            rf.write("\n".join(all_matches))
            rf.write("\n```\n</details>\n\n")
        else:
            rf.write(":heavy_check_mark: No string-based connections found\n\n")
    
    # Exit with 0 if no issues were found; otherwise, exit with non-zero.
    sys.exit(0 if count == 0 else 1)

if __name__ == "__main__":
    main()
