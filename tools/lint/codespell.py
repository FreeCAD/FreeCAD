#!/usr/bin/env python3
import argparse
import logging
import os
import shutil
import sys
import sysconfig
import urllib.request
from pathlib import Path

from defaults import DEFAULT_CODESPELL_IGNORE_WORDS_REL, DEFAULT_CODESPELL_SKIP, repo_root
from utils import (
    add_common_arguments,
    append_file,
    emit_problem_matchers,
    ensure_tool,
    expand_files,
    init_environment,
    run_command,
    write_file,
)


def download_dictionary(url: str, dest: str) -> bool:
    logging.info("Downloading dictionary from %s to %s", url, dest)
    try:
        urllib.request.urlretrieve(url, dest)
        return True
    except Exception as exc:
        logging.warning("Error downloading dictionary: %s", exc)
        return False


def resolve_tool_path(tool: str) -> str:
    p = shutil.which(tool)
    if p:
        return p

    try:
        scripts_dir = sysconfig.get_path("scripts") or ""
    except Exception:
        scripts_dir = ""

    if scripts_dir:
        candidate = Path(scripts_dir) / tool
        if candidate.is_file():
            return str(candidate)

    return tool


def generate_markdown_report(misspellings: int | None, log_file: str) -> str:
    report_lines = []
    if misspellings is None:
        report_lines.append(
            "<details><summary>:x: Codespell failed to run</summary>"
        )
    elif misspellings > 0:
        report_lines.append(
            f"<details><summary>:pencil2: Codespell found {misspellings} misspellings</summary>"
        )
    else:
        report_lines.append(
            "<details><summary>:heavy_check_mark: Codespell found no misspellings</summary>"
        )

    report_lines.append("")
    report_lines.append(
        "To ignore false positives, append the word to the [.github/codespellignore]"
        "(https://github.com/FreeCAD/FreeCAD/blob/master/.github/codespellignore) file (lowercase)"
    )
    report_lines.append("````")
    with open(log_file, "r", encoding="utf-8") as lf:
        report_lines.append(lf.read())
    report_lines.append("````")
    report_lines.append("</details>")
    report_lines.append("")
    return "\n".join(report_lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Run codespell on files and append a Markdown report."
    )
    add_common_arguments(parser)
    parser.add_argument(
        "--ignore-words",
        default="",
        help="Path to an ignore-words file (default: .github/codespellignore).",
    )
    parser.add_argument(
        "--skip",
        default="",
        help="Comma-separated list of file patterns to skip (default: repo defaults).",
    )
    parser.add_argument(
        "--write",
        action="store_true",
        help="Apply fixes in-place (codespell -w).",
    )
    args = parser.parse_args()
    init_environment(args)

    ensure_tool("codespell", package="codespell", prefer_pipx=False, check_args=["--version"])

    files = expand_files(args.files, only_existing=True)
    if not files:
        sys.exit(0)

    root = repo_root()

    ignore_words = (args.ignore_words or "").strip()
    if ignore_words:
        ignore_path = Path(ignore_words)
        if not ignore_path.is_absolute():
            ignore_path = (root / ignore_path).resolve()
    else:
        ignore_path = (root / DEFAULT_CODESPELL_IGNORE_WORDS_REL).resolve()

    if not ignore_path.is_file():
        logging.error("codespell ignore-words file not found: %s", ignore_path)
        sys.exit(2)

    skip = (args.skip or "").strip() or DEFAULT_CODESPELL_SKIP

    dictionary_file = os.path.join(args.log_dir, "codespell_dictionary.txt")
    use_dictionary = os.path.exists(dictionary_file)
    if not use_dictionary:
        dictionary_url = "https://raw.githubusercontent.com/codespell-project/codespell/master/codespell_lib/data/dictionary.txt"
        logging.info("Dictionary not found; downloading...")
        use_dictionary = download_dictionary(dictionary_url, dictionary_file)

    tool = resolve_tool_path("codespell")
    cmd = [
        tool,
        "--quiet-level",
        "3",
        "--summary",
        "--count",
        "--ignore-words",
        str(ignore_path),
        "--skip",
        skip,
    ]
    if args.write:
        cmd.append("-w")
    if use_dictionary and os.path.exists(dictionary_file):
        cmd += ["-D", dictionary_file]
    cmd += files

    stdout, stderr, exit_code = run_command(cmd, check=False)
    output = stdout + "\n" + stderr + "\n"

    log_file = os.path.join(args.log_dir, "codespell.log")
    write_file(log_file, output)
    emit_problem_matchers(log_file, "codespell.json", "codespell")
    def parse_misspellings(text: str) -> int | None:
        for raw in reversed(text.splitlines()):
            line = raw.strip()
            if line.isdigit():
                return int(line)
        return None

    misspellings = parse_misspellings(stdout + "\n" + stderr)

    report = generate_markdown_report(misspellings, log_file)
    append_file(args.report_file, report + "\n")

    if exit_code != 0 or misspellings is None:
        sys.exit(1)

    # In write mode, treat fixes as success.
    if args.write:
        sys.exit(0)

    sys.exit(0 if misspellings == 0 else 1)


if __name__ == "__main__":
    main()
