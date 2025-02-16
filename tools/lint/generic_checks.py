import argparse
import glob
import os
from utils import setup_logger


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
                lines = f.readlines()
                error_lines = []
                for idx, line in enumerate(lines, start=1):
                    if line.rstrip("\n") != line.rstrip():
                        error_lines.append(idx)
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
                lines = f.readlines()
                tab_lines = []
                for idx, line in enumerate(lines, start=1):
                    if "\t" in line:
                        tab_lines.append(idx)
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


def get_file_list(pattern):
    return glob.glob(pattern, recursive=True)


def main():
    parser = argparse.ArgumentParser(description="Run formatting lint checks.")
    parser.add_argument(
        "--files", required=True, help="Glob pattern for files to check."
    )
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
    parser.add_argument(
        "--log-dir", required=True, help="Directory in which to write log files."
    )
    parser.add_argument(
        "--report-file",
        required=True,
        help="Path to the Markdown report file to append results.",
    )
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output.")
    args = parser.parse_args()

    setup_logger(args.verbose)

    # Ensure directories exist.
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    file_list = get_file_list(args.files)
    file_list = [f for f in file_list if os.path.isfile(f)]

    report_sections = []

    # Check non-Unix line endings and emit warnings
    if args.lineendings_check:
        le_issues = check_line_endings(file_list)
        # Emit GitHub Actions warning commands for each file with non-Unix line endings.
        for file, detail in le_issues.items():
            # Print warning command: GitHub Actions will pick this up.
            print(f"::warning file={file},title={file}::{detail}")
        report_sections.append(format_report("Non-Unix Line Endings", le_issues))

    RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")

    # Check trailing whitespace and emit problem matcher output
    if args.whitespace_check:
        ws_issues = check_trailing_whitespace(file_list)
        if ws_issues:
            # Add problem matcher for trailing whitespace.
            print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/grepMatcherWarning.json")
            for file, details in ws_issues.items():
                if isinstance(details, list):
                    for line in details:
                        print(f"{file}:{line}: trailing whitespace")
                else:
                    print(f"{file}: {details}")
            print("::remove-matcher owner=grepMatcher-warning::")
        report_sections.append(format_report("Trailing Whitespace", ws_issues))

    # Check tab usage and emit problem matcher output
    if args.tabs_check:
        tab_issues = check_tabs(file_list)
        if tab_issues:
            # Add problem matcher for tab usage.
            print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/grepMatcherWarning.json")
            for file, details in tab_issues.items():
                if isinstance(details, list):
                    for line in details:
                        print(f"{file}:{line}: contains tab")
                else:
                    print(f"{file}: {details}")
            print("::remove-matcher owner=grepMatcher-warning::")
        report_sections.append(format_report("Tab Usage", tab_issues))

    report_content = "\n".join(report_sections)
    with open(args.report_file, "w", encoding="utf-8") as f:
        f.write(report_content)
    print("Lint report generated at:", args.report_file)


if __name__ == "__main__":
    main()
