import argparse
import glob
import os
from utils import (
    add_common_arguments,
    init_environment,
    write_file,
    emit_problem_matchers,
)


def check_line_endings(file_paths):
    """Check if files have non-Unix (CRLF or CR) line endings.

    Returns a dict mapping file path to an issue description.
    """
    issues = {}
    for path in file_paths:
        try:
            with open(path, "rb") as f:
                content = f.read()
                if b"\r\n" in content or (b"\r" in content and b"\n" not in content):
                    issues[path] = "File has non-Unix line endings."
        except Exception as e:
            issues[path] = f"Error reading file: {e}"
    return issues


def check_trailing_whitespace(file_paths):
    """Check for trailing whitespace in files.

    Returns a dict mapping file paths to a list of line numbers with trailing whitespace.
    """
    issues = {}
    for path in file_paths:
        try:
            with open(path, "r", encoding="utf-8") as f:
                lines = f.read().splitlines()
                error_lines = [
                    idx
                    for idx, line in enumerate(lines, start=1)
                    if line != line.rstrip()
                ]
                if error_lines:
                    issues[path] = error_lines
        except Exception as e:
            issues[path] = f"Error reading file: {e}"
    return issues


def check_tabs(file_paths):
    """Check for usage of tab characters in files.

    Returns a dict mapping file paths to a list of line numbers containing tabs.
    """
    issues = {}
    for path in file_paths:
        try:
            with open(path, "r", encoding="utf-8") as f:
                lines = f.read().splitlines()
                tab_lines = [
                    idx for idx, line in enumerate(lines, start=1) if "\t" in line
                ]
                if tab_lines:
                    issues[path] = tab_lines
        except Exception as e:
            issues[path] = f"Error reading file: {e}"
    return issues


def format_report(section_title, issues):
    """Format a report section with a Markdown details block."""
    report = []
    if issues:
        count = len(issues)
        report.append(
            f"<details><summary>:information_source: Found {count} issue(s) in {section_title}</summary>\n"
        )
        report.append("```")
        for file, details in issues.items():
            report.append(f"{file}: {details}")
        report.append("```\n</details>\n")
    else:
        report.append(f":heavy_check_mark: No issues found in {section_title}\n")
    return "\n".join(report)


def main():
    parser = argparse.ArgumentParser(description="Run formatting lint checks.")
    add_common_arguments(parser)
    parser.add_argument(
        "--lineendings-check",
        action="store_true",
        help="Enable check for non-Unix line endings.",
    )
    parser.add_argument(
        "--whitespace-check",
        action="store_true",
        help="Enable check for trailing whitespace.",
    )
    parser.add_argument(
        "--tabs-check", action="store_true", help="Enable check for tab characters."
    )
    args = parser.parse_args()
    init_environment(args)

    file_list = glob.glob(args.files, recursive=True)
    file_list = [f for f in file_list if os.path.isfile(f)]

    report_sections = []

    # Check non-Unix line endings.
    if args.lineendings_check:
        le_issues = check_line_endings(file_list)
        for file, detail in le_issues.items():
            print(f"::warning file={file},title={file}::{detail}")
        report_sections.append(format_report("Non-Unix Line Endings", le_issues))

    # Check trailing whitespace.
    if args.whitespace_check:
        ws_issues = check_trailing_whitespace(file_list)
        if ws_issues:
            ws_output_lines = []
            for file, details in ws_issues.items():
                if isinstance(details, list):
                    for line in details:
                        ws_output_lines.append(f"{file}:{line}: trailing whitespace")
                else:
                    ws_output_lines.append(f"{file}: {details}")
            ws_output = "\n".join(ws_output_lines)

            ws_log_file = os.path.join(args.log_dir, "whitespace.log")
            write_file(ws_log_file, ws_output)
            emit_problem_matchers(
                ws_log_file, "grepMatcherWarning.json", "grepMatcher-warning"
            )
        report_sections.append(format_report("Trailing Whitespace", ws_issues))

    # Check tab usage.
    if args.tabs_check:
        tab_issues = check_tabs(file_list)
        if tab_issues:
            tab_output_lines = []
            for file, details in tab_issues.items():
                if isinstance(details, list):
                    for line in details:
                        tab_output_lines.append(f"{file}:{line}: contains tab")
                else:
                    tab_output_lines.append(f"{file}: {details}")
            tab_output = "\n".join(tab_output_lines)

            tab_log_file = os.path.join(args.log_dir, "tabs.log")
            write_file(tab_log_file, tab_output)
            emit_problem_matchers(
                tab_log_file, "grepMatcherWarning.json", "grepMatcher-warning"
            )
        report_sections.append(format_report("Tab Usage", tab_issues))

    report_content = "\n".join(report_sections)
    write_file(args.report_file, report_content)
    print("Lint report generated at:", args.report_file)


if __name__ == "__main__":
    main()
