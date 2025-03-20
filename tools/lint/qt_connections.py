#!/usr/bin/env python3
import argparse
import os
import re
import sys
from utils import (
    add_common_arguments,
    init_environment,
    write_file,
    append_file,
    emit_problem_matchers,
)


def check_qt_connections(file_path):
    """
    Scan the file for lines containing 'SIGNAL' or 'SLOT' and return a list of matching lines
    with line numbers and a suggestion.
    """
    matches = []
    try:
        with open(file_path, "r", encoding="utf-8", errors="replace") as f:
            for lineno, line in enumerate(f, start=1):
                if re.search(r"\b(SIGNAL|SLOT)\b", line):
                    matches.append(
                        f"{file_path}:{lineno}: {line.rstrip()} <-- Consider using functor-based connections"
                    )
    except Exception as e:
        logging.debug(f"Error reading {file_path}: {e}")
    return matches


def generate_markdown_report(all_matches):
    """Generate a Markdown report based on the list of QT connection matches."""
    count = len(all_matches)
    if count > 0:
        md_report = (
            f"<details><summary>:information_source: Found {count} QT string-based connections → consider using QT functor-based connections</summary>\n"
            "\nFor more information see: https://wiki.qt.io/New_Signal_Slot_Syntax or https://github.com/FreeCAD/FreeCAD/issues/6166\n"
            "```\n" + "\n".join(all_matches) + "\n```\n</details>\n\n"
        )
    else:
        md_report = ":heavy_check_mark: No string-based connections found\n\n"
    return md_report


def main():
    parser = argparse.ArgumentParser(
        description="Check for old Qt string-based connections in files and append a Markdown report."
    )
    add_common_arguments(parser)
    args = parser.parse_args()
    init_environment(args)

    all_matches = []
    for file_path in args.files.split():
        logging.debug(f"Checking file: {file_path}")
        matches = check_qt_connections(file_path)
        all_matches.extend(matches)

    log_file = os.path.join(args.log_dir, "qtConnections.log")
    write_file(log_file, "\n".join(all_matches))
    emit_problem_matchers(log_file, "grepMatcherWarning.json", "grepMatcher-warning")

    count = len(all_matches)
    logging.info(f"Found {count} QT string-based connections")

    md_report = generate_markdown_report(all_matches)
    append_file(args.report_file, md_report)

    sys.exit(0 if count == 0 else 1)


if __name__ == "__main__":
    main()
