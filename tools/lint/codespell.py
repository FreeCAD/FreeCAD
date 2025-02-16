#!/usr/bin/env python3
import argparse
import subprocess
import sys
import os
import urllib.request

def log_verbose(message, verbose):
    if verbose:
        print(message)

def download_dictionary(url, dest, verbose=False):
    log_verbose(f"Downloading dictionary from {url} to {dest}", verbose)
    try:
        urllib.request.urlretrieve(url, dest)
    except Exception as e:
        print(f"Error downloading dictionary: {e}", file=sys.stderr)
        sys.exit(1)

def emit_codespell_problem_matchers(log_path: str):
    """Emit GitHub Actions problem matcher commands for codespell output."""
    if os.path.isfile(log_path):
        RUNNER_WORKSPACE = os.getenv("RUNNER_WORKSPACE")
        print(f"::add-matcher::{RUNNER_WORKSPACE}/FreeCAD/.github/problemMatcher/codespell.json")
        with open(log_path, "r", encoding="utf-8") as f:
            sys.stdout.write(f.read())
        print("::remove-matcher owner=codespell::")

def main():
    parser = argparse.ArgumentParser(
        description="Run codespell on files and append a Markdown report."
    )
    parser.add_argument("--files", required=True,
                        help="A space-separated list (or glob-expanded string) of files to check.")
    parser.add_argument("--ignore-words", required=True,
                        help="Comma-separated list of words to ignore (from codespellignore).")
    parser.add_argument("--skip", required=True,
                        help="Comma-separated list of file patterns to skip.")
    parser.add_argument("--log-dir", required=True,
                        help="Directory where the codespell log will be written.")
    parser.add_argument("--report-file", required=True,
                        help="Path to the Markdown report file to append results.")
    parser.add_argument("--verbose", action="store_true",
                        help="Enable verbose output.")
    args = parser.parse_args()
    verbose = args.verbose

    # Ensure log directory and report file directory exist.
    os.makedirs(args.log_dir, exist_ok=True)
    os.makedirs(os.path.dirname(args.report_file), exist_ok=True)

    # Check for dictionary.txt; download it if not present.
    dictionary_file = "dictionary.txt"
    if not os.path.exists(dictionary_file):
        dictionary_url = "https://raw.githubusercontent.com/codespell-project/codespell/master/codespell_lib/data/dictionary.txt"
        log_verbose("Dictionary not found; downloading...", verbose)
        download_dictionary(dictionary_url, dictionary_file, verbose=verbose)
    else:
        log_verbose("Dictionary file found.", verbose)

    # Install codespell quietly.
    subprocess.run("pip install -q codespell", shell=True, check=True)

    # Build the codespell command.
    # The original shell command is:
    # codespell --quiet-level 3 --summary --count --ignore-words $ignored --skip $skip -D dictionary.txt $files
    cmd = (
        f"codespell --quiet-level 3 --summary --count "
        f"--ignore-words {args.ignore_words} --skip {args.skip} "
        f"-D {dictionary_file} {args.files}"
    )
    log_verbose(f"Running command: {cmd}", verbose)
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)

    # Write codespell's output to the log file.
    log_file = os.path.join(args.log_dir, "codespell.log")
    with open(log_file, "w", encoding="utf-8") as lf:
        lf.write(result.stdout)
        lf.write(result.stderr)

    # Emit problem matcher commands so GitHub Actions can highlight codespell issues.
    emit_codespell_problem_matchers(log_file)

    # Try to parse the output as an integer (the misspellings count).
    try:
        misspellings = int(result.stdout.strip())
    except ValueError:
        # If parsing fails, print the output for debugging and default to 0.
        log_verbose(f"Could not parse misspellings count from output:\n{result.stdout}", verbose)
        misspellings = 0

    print(f"Found {misspellings} misspellings")

    # Build Markdown report section.
    report_lines = []
    if misspellings > 0:
        report_lines.append(f"<details><summary>:pencil2: Codespell found {misspellings} misspellings</summary>")
    else:
        report_lines.append("<details><summary>:heavy_check_mark: Codespell found no misspellings</summary>")
    report_lines.append("")
    report_lines.append("To ignore false positives, append the word to the [.github/codespellignore](https://github.com/FreeCAD/FreeCAD/blob/master/.github/codespellignore) file (lowercase)")
    report_lines.append("````")
    with open(log_file, "r", encoding="utf-8") as lf:
        report_lines.append(lf.read())
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")

    # Append the generated report to the report file.
    with open(args.report_file, "a", encoding="utf-8") as rf:
        rf.write("\n".join(report_lines) + "\n")

    # Exit with 0 if no misspellings, otherwise exit nonzero.
    sys.exit(0 if misspellings == 0 else 1)

if __name__ == "__main__":
    main()
