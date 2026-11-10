#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Check SPDX headers validities in the given list of directories.
Exit with code 1 if any failure is found.
"""
import argparse
from pathlib import Path
import re
import sys


SOURCE_SUFFIXES = {
    ".c", ".cpp", ".cxx", ".h", ".hpp", ".hxx",
    ".inl", ".py", ".pyi", ".cmake", "CMakeLists.txt",
}

# Subset of licenses used in the project
# Add any other from https://spdx.org/licenses/ if needed
LICENSES = {
    "BSD-3-Clause", "EUPL-1.2", "FTL", "GPL-3.0-only", "GPL-3.0-or-later",
    "LGPL-2.0-or-later", "LGPL-2.1-only", "LGPL-2.1-or-later", "LGPL-3.0-only",
    "MIT", "WTFPL", "Zlib",
}


def validate_license(expression: str) -> None:
    """Require a known license identifier somewhere in the line."""
    tokens = re.findall(r"[A-Za-z0-9][A-Za-z0-9.+-]*", expression)
    if not LICENSES.intersection(tokens):
        raise ValueError("no known SPDX license identifier")


def check_header(text: str) -> list[tuple[int, str]]:
    """Return (line, message) errors for missing or invalid tags."""
    lines = text.splitlines()
    licenses = []
    copyrights = []
    notices = []
    for number, line in enumerate(lines, 1):
        for tag, values in [("License-Identifier", licenses), ("FileCopyrightText", copyrights), ("FileNotice", notices)]:
            _, separator, value = line.partition(f"SPDX-{tag}:")
            if separator:
                values.append((number, value.strip()))

    errors = []
    if not licenses:
        errors.append((1, "missing SPDX-License-Identifier"))
    elif len(licenses) > 1:
        errors.append((licenses[1][0], "multiple SPDX-License-Identifier tags; use one expression"))
    for number, expression in licenses:
        try:
            validate_license(expression)
        except ValueError as error:
            errors.append((number, f"invalid SPDX license expression: {error}"))
    for number, value in copyrights:
        if not value:
            errors.append((number, "empty SPDX-FileCopyrightText"))
    for number, value in notices:
        if not value:
            errors.append((number, "empty SPDX-FileNotice"))
    return errors


def check_modules() -> bool:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("directories", nargs="+", help="directories to check")
    directories = parser.parse_args().directories

    modules = [Path(directory) for directory in directories]
    checked = 0
    failed = 0
    for module in modules:
        module_checked = 0
        module_failed = 0
        for path in sorted(module.rglob("*")):
            if not path.is_file() or not any(path.name.endswith(suffix) for suffix in SOURCE_SUFFIXES):
                continue
            module_checked += 1
            text = path.read_text(encoding="utf-8")
            errors = check_header(text)
            for number, message in errors:
                print(f"{path}:{number}: {message}", file=sys.stderr)
            module_failed += len(errors) > 0
        print(f"{"FAIL" if module_failed else "PASS"}: {module} ({module_checked} files checked, {module_failed} failed)", flush=True)
        checked += module_checked
        failed += module_failed

    print(f"SPDX: checked {checked} files; {failed} failed.")
    return failed == 0


if __name__ == "__main__":
    sys.exit(0 if check_modules() else 1)
